#include "texobj.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"


static GLboolean intelIsTextureResident(GLcontext *ctx,
                                      struct gl_texture_object *texObj)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   
   return 
      intelObj->mt && 
      intelObj->mt->region && 
      intel_is_region_resident(intel, intelObj->mt->region);
#endif
   return 1;
}



static struct gl_texture_image *intelNewTextureImage( GLcontext *ctx )
{
   (void) ctx;
   return (struct gl_texture_image *)CALLOC_STRUCT(intel_texture_image);
}


static struct gl_texture_object *intelNewTextureObject( GLcontext *ctx, 
							GLuint name, 
							GLenum target )
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}


static void intelFreeTextureImageData( GLcontext *ctx, 
				     struct gl_texture_image *texImage )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
   }
   
   if (texImage->Data) {
      free(texImage->Data);
      texImage->Data = NULL;
   }
}


#ifndef __x86_64__
static unsigned
fastrdtsc(void)
{
    unsigned eax;
    __asm__ volatile ("\t"
	"pushl  %%ebx\n\t"
	"cpuid\n\t" ".byte 0x0f, 0x31\n\t" "popl %%ebx\n":"=a" (eax)
	:"0"(0)
	:"ecx", "edx", "cc");

    return eax;
}
#else
static unsigned
fastrdtsc(void)
{
    unsigned eax;
    __asm__ volatile ("\t"
	"cpuid\n\t" ".byte 0x0f, 0x31\n\t" :"=a" (eax)
	:"0"(0)
		      :"ecx", "edx", "ebx", "cc");

    return eax;
}
#endif

static unsigned
time_diff(unsigned t, unsigned t2)
{
    return ((t < t2) ? t2 - t : 0xFFFFFFFFU - (t - t2 - 1));
}


/* The system memcpy (at least on ubuntu 5.10) has problems copying
 * to agp (writecombined) memory from a source which isn't 64-byte
 * aligned - there is a 4x performance falloff.
 *
 * The x86 __memcpy is immune to this but is slightly slower
 * (10%-ish) than the system memcpy.
 *
 * The sse_memcpy seems to have a slight cliff at 64/32 bytes, but
 * isn't much faster than x86_memcpy for agp copies.
 * 
 * TODO: switch dynamically.
 */
static void *do_memcpy( void *dest, const void *src, size_t n )
{
<<<<<<< intel_tex.c

   if (!image || !image->Data) 
      return;

   if (image->Depth == 1 && image->IsClientData) {
      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "Blit uploading\n");

      /* Do it with a blit.
       */
      intelEmitCopyBlitLocked( intel,
			       image->TexFormat->TexelBytes,
			       image->RowStride, /* ? */
			       intelGetMemoryOffsetMESA( NULL, 0, image->Data ),
			       t->Pitch / image->TexFormat->TexelBytes,
			       intelGetMemoryOffsetMESA( NULL, 0, t->BufAddr + offset ),
			       0, 0,
			       0, 0,
			       image->Width,
			       image->Height);
   }
   else if (image->IsCompressed) {
      GLuint row_len = image->Width * 2;
      GLubyte *dst = (GLubyte *)(t->BufAddr + offset);
      GLubyte *src = (GLubyte *)image->Data;
      GLuint j;

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, 
		 "Upload image %dx%dx%d offset %xm row_len %x "
		 "pitch %x depth_pitch %x\n",
		 image->Width, image->Height, image->Depth, offset,
		 row_len, t->Pitch, t->depth_pitch);

      switch (image->InternalFormat) {
	case GL_COMPRESSED_RGB_FXT1_3DFX:
	case GL_COMPRESSED_RGBA_FXT1_3DFX:
	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	  for (j = 0 ; j < image->Height/4 ; j++, dst += (t->Pitch)) {
	    __memcpy(dst, src, row_len );
	    src += row_len;
	  }
	  break;
	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	  for (j = 0 ; j < image->Height/4 ; j++, dst += (t->Pitch)) {
	    __memcpy(dst, src, (image->Width*4) );
	    src += image->Width*4;
	  }
	  break;
	default:
	  fprintf(stderr,"Internal Compressed format not supported %d\n", image->InternalFormat);
	  break;
      }
   }
   /* Time for another vtbl entry:
    */
   else if (intel->intelScreen->deviceID == PCI_CHIP_I945_G ||
            intel->intelScreen->deviceID == PCI_CHIP_I945_GM) {
      GLuint row_len = image->Width * image->TexFormat->TexelBytes;
      GLubyte *dst = (GLubyte *)(t->BufAddr + offset);
      GLubyte *src = (GLubyte *)image->Data;
      GLuint d, j;

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, 
		 "Upload image %dx%dx%d offset %xm row_len %x "
		 "pitch %x depth_pitch %x\n",
		 image->Width, image->Height, image->Depth, offset,
		 row_len, t->Pitch, t->depth_pitch);

      if (row_len == t->Pitch) {
	 memcpy( dst, src, row_len * image->Height * image->Depth );
      }
      else { 
	 GLuint x = 0, y = 0;

	 for (d = 0 ; d < image->Depth ; d++) {
	    GLubyte *dst0 = dst + x + y * t->Pitch;

	    for (j = 0 ; j < image->Height ; j++) {
	       __memcpy(dst0, src, row_len );
	       src += row_len;
	       dst0 += t->Pitch;
	    }

	    x += MIN2(4, row_len); /* Guess: 4 byte minimum alignment */
	    if (x > t->Pitch) {
	       x = 0;
	       y += image->Height;
	    }
	 }
      }

   }
   else {
      GLuint row_len = image->Width * image->TexFormat->TexelBytes;
      GLubyte *dst = (GLubyte *)(t->BufAddr + offset);
      GLubyte *src = (GLubyte *)image->Data;
      GLuint d, j;

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, 
		 "Upload image %dx%dx%d offset %xm row_len %x "
		 "pitch %x depth_pitch %x\n",
		 image->Width, image->Height, image->Depth, offset,
		 row_len, t->Pitch, t->depth_pitch);

      if (row_len == t->Pitch) {
	 for (d = 0; d < image->Depth; d++) {
	    memcpy( dst, src, t->Pitch * image->Height );
	    dst += t->depth_pitch;
	    src += row_len * image->Height;
	 }
      }
      else { 
	 for (d = 0 ; d < image->Depth ; d++) {
	    for (j = 0 ; j < image->Height ; j++) {
	       __memcpy(dst, src, row_len );
	       src += row_len;
	       dst += t->Pitch;
	    }

	    dst += t->depth_pitch - (t->Pitch * image->Height);
	 }
      }
   }
=======
   if ( (((unsigned)src) & 63) ||
	(((unsigned)dest) & 63)) {
      return  __memcpy(dest, src, n);	
   }
   else
      return memcpy(dest, src, n);
>>>>>>> 1.10.2.8
}


static void *timed_memcpy( void *dest, const void *src, size_t n )
{
   void *ret;
   unsigned t1, t2;
   double rate;

   if ( (((unsigned)src) & 63) ||
	(((unsigned)dest) & 63)) 
      _mesa_printf("Warning - non-aligned texture copy!\n");

   t1 = fastrdtsc();
   ret =  do_memcpy(dest, src, n);	
   t2 = fastrdtsc();

   rate = time_diff(t1, t2);
   rate /= (double) n;
   _mesa_printf("timed_memcpy: %u %u --> %f clocks/byte\n", t1, t2, rate); 
   return ret;
}


void intelInitTextureFuncs(struct dd_function_table * functions)
{
   functions->ChooseTextureFormat = intelChooseTextureFormat;
   functions->TexImage1D = intelTexImage1D;
   functions->TexImage2D = intelTexImage2D;
   functions->TexImage3D = intelTexImage3D;
   functions->TexSubImage1D = intelTexSubImage1D;
   functions->TexSubImage2D = intelTexSubImage2D;
   functions->TexSubImage3D = intelTexSubImage3D;
   functions->CopyTexImage1D = intelCopyTexImage1D;
   functions->CopyTexImage2D = intelCopyTexImage2D;
   functions->CopyTexSubImage1D = intelCopyTexSubImage1D;
   functions->CopyTexSubImage2D = intelCopyTexSubImage2D;
   functions->GetTexImage = intelGetTexImage;
   functions->NewTextureObject = intelNewTextureObject;
   functions->NewTextureImage = intelNewTextureImage;
   functions->DeleteTexture = _mesa_delete_texture_object;
   functions->FreeTexImageData = intelFreeTextureImageData;
   functions->UpdateTexturePalette = 0;
   functions->IsTextureResident = intelIsTextureResident;

   if (INTEL_DEBUG & DEBUG_BUFMGR)
      functions->TextureMemCpy = timed_memcpy;
   else
      functions->TextureMemCpy = do_memcpy;
}
