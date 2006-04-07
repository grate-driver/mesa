/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_bufmgr.h"
#include "enums.h"

static GLenum target_to_target( GLenum target )
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return GL_TEXTURE_CUBE_MAP_ARB;
   default:
      return target;
   }
}

struct intel_mipmap_tree *intel_miptree_create( struct intel_context *intel,
						GLenum target,
						GLenum internal_format,
						GLuint first_level,
						GLuint last_level,
						GLuint width0,
						GLuint height0,
						GLuint depth0,
						GLuint cpp,
						GLboolean compressed)
{
   GLboolean ok;
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);

   DBG("%s target %s format %s level %d..%d\n", __FUNCTION__,
		_mesa_lookup_enum_by_nr(target),
		_mesa_lookup_enum_by_nr(internal_format),
		first_level,
		last_level);

   mt->target = target_to_target(target);
   mt->internal_format = internal_format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->width0 = width0;
   mt->height0 = height0;
   mt->depth0 = depth0;
   mt->cpp = cpp;
   mt->compressed = compressed;
   mt->refcount = 1;

   switch (intel->intelScreen->deviceID) {
   case PCI_CHIP_I945_G:
/*    case PCI_CHIP_I945_GM: */
      ok = i945_miptree_layout( mt );
      break;
   default:
      /* All the i830 chips and the i915 use this layout:
       */
      ok = i915_miptree_layout( mt );
      break;
   }

   if (ok)
      mt->region = intel_region_alloc( intel, 
				       mt->cpp,
				       mt->pitch, 
				       mt->total_height );

   if (!mt->region) {
      free(mt);
      return NULL;
   }

   return mt;
}


void intel_miptree_reference( struct intel_mipmap_tree **dst, 
			      struct intel_mipmap_tree *src )
{
   src->refcount++;
   *dst = src;
}

void intel_miptree_release( struct intel_context *intel,
			    struct intel_mipmap_tree **mt )
{
   if (!*mt)
      return;

   DBG("%s %d\n", __FUNCTION__, (*mt)->refcount-1);
   if (--(*mt)->refcount == 0) {
      intel_region_release(intel, &((*mt)->region));
      free(*mt);
   }
   *mt = NULL;
}




/* Can the image be pulled into a unified mipmap tree.  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
GLboolean intel_miptree_match_image( struct intel_mipmap_tree *mt, 
				     struct gl_texture_image *image,
				     GLuint face,
				     GLuint level )
{
   DBG("%s %d %d/%d %d/%d\n", __FUNCTION__,
		image->Border,
		image->InternalFormat, mt->internal_format,
		image->IsCompressed, mt->compressed);

   /* Images with borders are never pulled into mipmap trees. 
    */
   if (image->Border)
      return GL_FALSE;

   if (image->InternalFormat != mt->internal_format ||
       image->IsCompressed != mt->compressed)
      return GL_FALSE;

   DBG("%s: %d/%d %d/%d %d/%d\n", __FUNCTION__,
		image->Width, mt->offset[face][level].width,
		image->Height, mt->offset[face][level].height,
		image->Depth, mt->offset[face][level].depth);

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (image->Width != mt->offset[face][level].width ||
       image->Height != mt->offset[face][level].height ||
       image->Depth != mt->offset[face][level].depth)
      return GL_FALSE;


   DBG("%s: success\n", __FUNCTION__);
   return GL_TRUE;
}


GLuint intel_miptree_image_offset(struct intel_mipmap_tree *mt,
				  GLuint face,
				  GLuint level)
{
   return mt->offset[face][level].offset;
}

GLuint intel_miptree_depth_image_stride(struct intel_mipmap_tree *mt,
					GLuint face,
					GLuint level)
{
   return mt->offset[face][level].depth_image_stride;
}


void intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
				    GLuint face,
				    GLuint level,
				    GLuint x, GLuint y,
				    GLuint w, GLuint h, GLuint d)
{
   mt->offset[face][level].offset = (x + y * mt->pitch) * mt->cpp;
   mt->offset[face][level].width = w;
   mt->offset[face][level].height = h;
   mt->offset[face][level].depth = d;
}



/**
 * Map a teximage in a mipmap tree.
 * \param row_stride  returns row stride in bytes
 * \param image_stride  returns image stride in bytes (for 3D textures).
 * \return address of mapping
 */
GLubyte *intel_miptree_image_map(struct intel_context *intel, 
				 struct intel_mipmap_tree *mt,
				 GLuint face,
				 GLuint level,
				 GLuint *row_stride,
				 GLuint *image_stride)
{
   DBG("%s \n", __FUNCTION__);
   
   if (row_stride)
      *row_stride = mt->pitch * mt->cpp;
   
   if (image_stride) 
      *image_stride = mt->offset[face][level].depth_image_stride;

   return (intel_region_map(intel, mt->region) +
	   intel_miptree_image_offset(mt, face, level));
}

void intel_miptree_image_unmap(struct intel_context *intel, 
			       struct intel_mipmap_tree *mt)
{
   DBG("%s\n", __FUNCTION__);
   intel_region_unmap(intel, mt->region);
}



/* Upload data for a particular image.
 */
void intel_miptree_image_data(struct intel_context *intel, 
			      struct intel_mipmap_tree *dst,
			      GLuint face,
			      GLuint level,
			      void *src, 
			      GLuint src_row_pitch,
			      GLuint src_image_pitch)
{
   GLuint depth = dst->offset[face][level].depth;
   GLuint dst_offset = intel_miptree_image_offset(dst, face, level);
   GLuint dst_image_stride = intel_miptree_depth_image_stride(dst, face, level);
   GLuint i;

   DBG("%s\n", __FUNCTION__);
   for (i = 0; i < depth; i++) {
      intel_region_data(intel,
			dst->region, dst_offset,
			0,
			0,
			src,
			src_row_pitch,
			0, 0,	/* source x,y */
			dst->offset[face][level].width,
			dst->offset[face][level].height);
      dst_offset += dst_image_stride;
      src += src_image_pitch;
   }
}
			  
/* Copy mipmap image between trees
 */
void intel_miptree_image_copy( struct intel_context *intel,
			       struct intel_mipmap_tree *dst,
			       GLuint face, GLuint level,
			       struct intel_mipmap_tree *src )
{
   GLuint width = src->offset[face][level].width;
   GLuint height = src->offset[face][level].height;
   GLuint depth = src->offset[face][level].depth;
   GLuint dst_offset = intel_miptree_image_offset(dst, face, level);
   GLuint src_offset = intel_miptree_image_offset(src, face, level);
   GLuint dst_image_stride = intel_miptree_depth_image_stride(dst, face, level);
   GLuint src_image_stride = intel_miptree_depth_image_stride(src, face, level);
   GLuint i;

   for (i = 0; i < depth; i++) {
      intel_region_copy(intel,
			dst->region, dst_offset,
			0,
			0,
			src->region, src_offset,
			0,
			0,
			width,
			height);

      dst_offset += dst_image_stride;
      src_offset += src_image_stride;
   }
		     
}
