
#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "enums.h"
#include "colortab.h"
#include "convolve.h"
#include "context.h"
#include "simple_list.h"
#include "texcompress.h"
#include "texformat.h"
#include "texobj.h"
#include "texstore.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"
#include "intel_ioctl.h"


/* Functions to store texture images.  Where possible, mipmap_tree's
 * will be created or further instantiated with image data, otherwise
 * images will be stored in malloc'd memory.  A validation step is
 * required to pull those images into a mipmap tree, or otherwise
 * decide a fallback is required.
 */


static int logbase2(int n)
{
   GLint i = 1;
   GLint log2 = 0;

   while (n > i) {
      i *= 2;
      log2++;
   }

   return log2;
}


/* Otherwise, store it in memory if (Border != 0) or (any dimension ==
 * 1).
 *    
 * Otherwise, if max_level >= level >= min_level, create tree with
 * space for textures from min_level down to max_level.
 *
 * Otherwise, create tree with space for textures from (level
 * 0)..(1x1).  Consider pruning this tree at a validation if the
 * saving is worth it.
 */
static void guess_and_alloc_mipmap_tree( struct intel_context *intel,
					 struct intel_texture_object *intelObj,
					 struct intel_texture_image *intelImage )
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = intelImage->base.Width;
   GLuint height = intelImage->base.Height;
   GLuint depth = intelImage->base.Depth;
   GLuint l2width, l2height, l2depth;
   GLuint i;

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Border)
      return;

   if (intelImage->level > intelObj->base.BaseLevel &&
       (intelImage->base.Width == 1 ||
	(intelObj->base.Target != GL_TEXTURE_1D && 
	 intelImage->base.Height == 1) ||
	(intelObj->base.Target == GL_TEXTURE_3D &&
	 intelImage->base.Depth == 1)))
      return;

   /* If this image disrespects BaseLevel, allocate from level zero.
    * Usually BaseLevel == 0, so it's unlikely to happen.
    */
   if (intelImage->level < intelObj->base.BaseLevel)
      firstLevel = 0;
   else
      firstLevel = intelObj->base.BaseLevel;


   /* Figure out image dimensions at start level. 
    */
   for (i = intelImage->level; i > firstLevel; i--) {
      width <<= 1;
      if (height != 1) height <<= 1;
      if (depth != 1) depth <<= 1;
   }

   /* Guess a reasonable value for lastLevel.  This is probably going
    * to be wrong fairly often and might mean that we have to look at
    * resizable buffers, or require that buffers implement lazy
    * pagetable arrangements.
    */
   if ((intelObj->base.MinFilter == GL_NEAREST || 
	intelObj->base.MinFilter == GL_LINEAR) &&
       intelImage->level == firstLevel) {
      lastLevel = firstLevel;
   }
   else {
      l2width = logbase2(width);
      l2height = logbase2(height);
      l2depth = logbase2(depth);
      lastLevel = firstLevel + MAX2(MAX2(l2width,l2height),l2depth);
   }
	 
   assert(!intelObj->mt);
   intelObj->mt = intel_miptree_create( intel,
					intelObj->base.Target,
					intelImage->base.InternalFormat,
					firstLevel,
					lastLevel,
					width,
					height,
					depth,
					intelImage->base.TexFormat->TexelBytes,
					intelImage->base.IsCompressed );

   DBG("%s - success\n", __FUNCTION__);
}
   



static GLuint target_to_face( GLenum target )
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return ((GLuint) target - 
	      (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X);
   default:
      return 0;
   }
}


static void intelTexImage(GLcontext *ctx, 
			  GLint dims,
			  GLenum target, GLint level,
			  GLint internalFormat,
			  GLint width, GLint height, GLint depth,
			  GLint border,
			  GLenum format, GLenum type, const void *pixels,
			  const struct gl_pixelstore_attrib *unpack,
			  struct gl_texture_object *texObj,
			  struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLint postConvWidth = width;
   GLint postConvHeight = height;
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride;
   GLuint dstImageStride;


   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
		_mesa_lookup_enum_by_nr(target),
		level,
		width, height, depth, border);

   intelFlush(ctx);

   intelImage->face = target_to_face( target );
   intelImage->level = level;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
                                         &postConvHeight);
   }

   /* choose the texture format */
   texImage->TexFormat = intelChooseTextureFormat(ctx, internalFormat, 
						  format, type);

   assert(texImage->TexFormat);

   switch (dims) {
   case 1:
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel1D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel1Df;
      break;
   case 2:
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel2D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel2Df;
      break;
   case 3:
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel3D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel3Df;
      break;
   default:
      assert(0);
      break;
   }

   texelBytes = texImage->TexFormat->TexelBytes;


   /* Minimum pitch of 32 bytes */
   if (postConvWidth * texelBytes < 32) {
      postConvWidth = 32 / texelBytes;
      texImage->RowStride = postConvWidth;
   }

   assert(texImage->RowStride == postConvWidth);

   /* Release the reference to a potentially orphaned buffer.   
    * Release any old malloced memory.
    */
   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
      assert(!texImage->Data);
   }
   else if (texImage->Data) {
      free(texImage->Data);
   }

   /* If this is the only texture image in the tree, could call
    * bmBufferData with NULL data to free the old block and avoid
    * waiting on any outstanding fences.
    */
   if (intelObj->mt && 
       intelObj->mt->first_level == level &&
       intelObj->mt->last_level == level &&
       intelObj->mt->target != GL_TEXTURE_CUBE_MAP_ARB &&
       !intel_miptree_match_image(intelObj->mt, &intelImage->base,
				 intelImage->face, intelImage->level)) {

      DBG("release it\n");
      intel_miptree_release(intel, &intelObj->mt); 
      assert(!intelObj->mt);
   }
   
   if (!intelObj->mt) {
      guess_and_alloc_mipmap_tree(intel, intelObj, intelImage);
      if (!intelObj->mt)
	 _mesa_printf("guess_and_alloc_mipmap_tree: failed\n");
   }


   if (intelObj->mt && 
       intelObj->mt != intelImage->mt &&
       intel_miptree_match_image(intelObj->mt, &intelImage->base,
				 intelImage->face, intelImage->level)) {
      
      if (intelImage->mt) {
	 intel_miptree_release(intel, &intelImage->mt);
      }

      intel_miptree_reference(&intelImage->mt, intelObj->mt);
      assert(intelImage->mt);
   }

   if (!intelImage->mt)
      _mesa_printf("XXX: Image did not fit into tree - storing in local memory!\n");

   /* intelCopyTexImage calls this function with pixels == NULL, with
    * the expectation that the mipmap tree will be set up but nothing
    * more will be done.  This is where those calls return:
    */
   pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, 1, 
					format, type,
					pixels, unpack, "glTexImage");
   if (!pixels) 
      return;
   

   LOCK_HARDWARE(intel);
   
   if (intelImage->mt) {
      texImage->Data = intel_miptree_image_map(intel, 
					       intelImage->mt, 
					       intelImage->face, 
					       intelImage->level, 
					       &dstRowStride,
					       &dstImageStride);	 
   }
   else {
      /* Allocate regular memory and store the image there temporarily.   */
      if (texImage->IsCompressed) {
	 sizeInBytes = texImage->CompressedSize;
         dstRowStride = _mesa_compressed_row_stride(texImage->InternalFormat,width);
	 dstImageStride = 0;	/* ? */
	 assert(dims != 3);
      }
      else {
         dstRowStride = postConvWidth * texelBytes;
	 dstImageStride = dstRowStride * postConvHeight;
	 sizeInBytes = depth * dstImageStride;
      }
      texImage->Data = malloc(sizeInBytes);
   }

   if (INTEL_DEBUG & DEBUG_TEXTURE)
      _mesa_printf("Upload image %dx%dx%d row_len %x "
		   "pitch %x depth_pitch %x\n",
		   width, height, depth,
		   width * texelBytes, dstRowStride, dstImageStride);
     
   /* Copy data.  Would like to know when it's ok for us to eg. use
    * the blitter to copy.  Or, use the hardware to do the format
    * conversion and copy:
    */
   if (!texImage->TexFormat->StoreImage(ctx, dims,
					texImage->_BaseFormat,
					texImage->TexFormat,
					texImage->Data,
					0, 0, 0,  /* dstX/Y/Zoffset */
					dstRowStride, dstImageStride,
					width, height, depth,
					format, type, pixels, unpack)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
   }

   _mesa_unmap_teximage_pbo(ctx, unpack);

   if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      texImage->Data = NULL;
   }

   UNLOCK_HARDWARE(intel);

#if 0
   /* GL_SGIS_generate_mipmap -- this can be accelerated now.
    */
   if (level == texObj->BaseLevel && 
       texObj->GenerateMipmap) {
      intel_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif
}

void intelTexImage3D(GLcontext *ctx, 
		     GLenum target, GLint level,
		     GLint internalFormat,
		     GLint width, GLint height, GLint depth,
		     GLint border,
		     GLenum format, GLenum type, const void *pixels,
		     const struct gl_pixelstore_attrib *unpack,
		     struct gl_texture_object *texObj,
		     struct gl_texture_image *texImage)
{
   intelTexImage( ctx, 3, target, level, 
		  internalFormat, width, height, depth, border,
		  format, type, pixels,
		  unpack, texObj, texImage );
}


void intelTexImage2D(GLcontext *ctx, 
		     GLenum target, GLint level,
		     GLint internalFormat,
		     GLint width, GLint height, GLint border,
		     GLenum format, GLenum type, const void *pixels,
		     const struct gl_pixelstore_attrib *unpack,
		     struct gl_texture_object *texObj,
		     struct gl_texture_image *texImage)
{
   intelTexImage( ctx, 2, target, level, 
		  internalFormat, width, height, 1, border,
		  format, type, pixels,
		  unpack, texObj, texImage );
}

void intelTexImage1D(GLcontext *ctx, 
		     GLenum target, GLint level,
		     GLint internalFormat,
		     GLint width, GLint border,
		     GLenum format, GLenum type, const void *pixels,
		     const struct gl_pixelstore_attrib *unpack,
		     struct gl_texture_object *texObj,
		     struct gl_texture_image *texImage)
{
   intelTexImage( ctx, 1, target, level, 
		  internalFormat, width, 1, 1, border,
		  format, type, pixels,
		  unpack, texObj, texImage );
}



/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
void intelGetTexImage( GLcontext *ctx, GLenum target, GLint level,
                       GLenum format, GLenum type, GLvoid *pixels,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage )
{
   struct intel_context *intel = intel_context( ctx );
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   /* Map */
#ifdef ALL_IMAGES /* XXX Remove this, just for debug/test */
   intel_tex_map_images(intel, intel_texture_object(texObj));
#else
   /* XXX what if intelImage->mt is NULL? */
   if (intelImage->mt) {
      intelImage->base.Data = 
         intel_miptree_image_map(intel, 
                                 intelImage->mt,
                                 intelImage->face,
                                 intelImage->level,
                                 &intelImage->base.RowStride,
                                 &intelImage->base.ImageStride);
   }
#endif

   _mesa_get_teximage(ctx, target, level, format, type, pixels,
                      texObj, texImage);

   /* Unmap */
#ifdef ALL_IMAGES
   intel_tex_unmap_images(intel, intel_texture_object(texObj));
#else
   if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      intelImage->base.Data = NULL;
   }
#endif
}

