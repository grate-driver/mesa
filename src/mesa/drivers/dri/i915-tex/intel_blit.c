/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include <stdio.h>
#include <errno.h>

#include "mtypes.h"
#include "context.h"
#include "enums.h"

#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_context.h"
#include "intel_fbo.h"
#include "intel_reg.h"
#include "intel_regions.h"

#include "intel_bufmgr.h"


/**
 * Copy the back color buffer to the front color buffer. 
 * Used for SwapBuffers().
 */
void intelCopyBuffer( const __DRIdrawablePrivate *dPriv ) 
{
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel;

   DBG("%s\n", __FUNCTION__);

   assert(dPriv);

   /* We need a rendering context in order to issue the blit cmd.
    * Use the current context.
    * XXX need to fix this someday.
    */
   if (!ctx) {
      _mesa_problem(NULL, "No current context in intelCopyBuffer()");
      return;
   }
   intel = (struct intel_context *) ctx;

   bmFinishFence(intel->bm, intel->last_swap_fence);

   /* The LOCK_HARDWARE is required for the cliprects.  Buffer offsets
    * should work regardless.
    */
   LOCK_HARDWARE( intel );

   if (intel->driDrawable &&
       intel->driDrawable->numClipRects)
   {
      const intelScreenPrivate *intelScreen = intel->intelScreen;
      struct gl_framebuffer *fb
         = (struct gl_framebuffer *) dPriv->driverPrivate;
      const struct intel_region *frontRegion
         = intel_get_rb_region(fb, BUFFER_FRONT_LEFT);
      const struct intel_region *backRegion
         = intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      const int nbox = dPriv->numClipRects;
      const drm_clip_rect_t *pbox = dPriv->pClipRects;
      const int pitch = frontRegion->pitch;
      const int cpp = frontRegion->cpp;
      int BR13, CMD;
      int i;

      ASSERT(fb);
      ASSERT(fb->Name == 0); /* Not a user-created FBO */
      ASSERT(frontRegion);
      ASSERT(backRegion);
      ASSERT(frontRegion->pitch == backRegion->pitch);
      ASSERT(frontRegion->cpp == backRegion->cpp);

      if (cpp == 2) {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1<<24);
	 CMD = XY_SRC_COPY_BLT_CMD;
      } 
      else {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1<<24) | (1<<25);
	 CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		XY_SRC_COPY_BLT_WRITE_RGB);
      }

      for (i = 0 ; i < nbox; i++, pbox++) 
      {
	 if (pbox->x1 > pbox->x2 ||
	     pbox->y1 > pbox->y2 ||
	     pbox->x2 > intelScreen->width ||
	     pbox->y2 > intelScreen->height)
	    continue;
 
	 BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
	 OUT_BATCH( CMD );
	 OUT_BATCH( BR13 );
	 OUT_BATCH( (pbox->y1 << 16) | pbox->x1 );
	 OUT_BATCH( (pbox->y2 << 16) | pbox->x2 );

	 if (intel->sarea->pf_current_page == 0) 
	    OUT_RELOC( frontRegion->buffer, DRM_MM_TT|DRM_MM_WRITE, 0 );
	 else
	    OUT_RELOC( backRegion->buffer, DRM_MM_TT|DRM_MM_WRITE, 0 ); 
	 OUT_BATCH( (pbox->y1 << 16) | pbox->x1 );
	 OUT_BATCH( BR13 & 0xffff );

	 if (intel->sarea->pf_current_page == 0) 
	    OUT_RELOC( backRegion->buffer, DRM_MM_TT|DRM_MM_READ, 0 ); 
	 else
	    OUT_RELOC( frontRegion->buffer, DRM_MM_TT|DRM_MM_READ, 0 );

	 ADVANCE_BATCH();
      }

      intel->last_swap_fence = intel_batchbuffer_flush( intel->batch );
   }
   UNLOCK_HARDWARE( intel );
}




void intelEmitFillBlit( struct intel_context *intel,
			GLuint cpp,
			GLshort dst_pitch,
			GLuint dst_buffer,
			GLuint dst_offset,
			GLshort x, GLshort y, 
			GLshort w, GLshort h,
			GLuint color )
{
   GLuint BR13, CMD;
   BATCH_LOCALS;

   dst_pitch *= cpp;

   switch(cpp) {
   case 1: 
   case 2: 
   case 3: 
      BR13 = dst_pitch | (0xF0 << 16) | (1<<24);
      CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = dst_pitch | (0xF0 << 16) | (1<<24) | (1<<25);
      CMD = (XY_COLOR_BLT_CMD | XY_COLOR_BLT_WRITE_ALPHA |
	     XY_COLOR_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
   OUT_BATCH( CMD );
   OUT_BATCH( BR13 );
   OUT_BATCH( (y << 16) | x );
   OUT_BATCH( ((y+h) << 16) | (x+w) );
   OUT_RELOC( dst_buffer, DRM_MM_TT|DRM_MM_WRITE, dst_offset );
   OUT_BATCH( color );
   ADVANCE_BATCH();
}


/* Copy BitBlt
 */
void intelEmitCopyBlit( struct intel_context *intel,
			GLuint cpp,
			GLshort src_pitch,
			GLuint  src_buffer,
			GLuint  src_offset,
			GLshort dst_pitch,
			GLuint  dst_buffer,
			GLuint  dst_offset,
			GLshort src_x, GLshort src_y,
			GLshort dst_x, GLshort dst_y,
			GLshort w, GLshort h )
{
   GLuint CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   BATCH_LOCALS;


   DBG("%s src:buf(%d)/%d+%d %d,%d dst:buf(%d)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_offset, src_x, src_y,
       dst_buffer, dst_pitch, dst_offset, dst_x, dst_y,
       w,h);

   src_pitch *= cpp;
   dst_pitch *= cpp;

   switch(cpp) {
   case 1: 
   case 2: 
   case 3: 
      BR13 = (((GLint)dst_pitch)&0xffff) | (0xCC << 16) | (1<<24);
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 = (((GLint)dst_pitch)&0xffff) | (0xCC << 16) | (1<<24) | (1<<25);
      CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
	     XY_SRC_COPY_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   if (dst_y2 < dst_y ||
       dst_x2 < dst_x) {
      return;
   }

   /* Initial y values don't seem to work with negative pitches.  If
    * we adjust the offsets manually (below), it seems to work fine.
    *
    * On the other hand, if we always adjust, the hardware doesn't
    * know which blit directions to use, so overlapping copypixels get
    * the wrong result.
    */
   if ( dst_pitch > 0 && 
	src_pitch > 0) {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH( CMD );
      OUT_BATCH( BR13 );
      OUT_BATCH( (dst_y << 16) | dst_x );
      OUT_BATCH( (dst_y2 << 16) | dst_x2 );
      OUT_RELOC( dst_buffer, DRM_MM_TT|DRM_MM_WRITE, dst_offset );	
      OUT_BATCH( (src_y << 16) | src_x );
      OUT_BATCH( ((GLint)src_pitch&0xffff) );
      OUT_RELOC( src_buffer, DRM_MM_TT|DRM_MM_READ, src_offset ); 
      ADVANCE_BATCH();
   }
   else {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH( CMD );
      OUT_BATCH( BR13 );
      OUT_BATCH( (0 << 16) | dst_x );
      OUT_BATCH( (h << 16) | dst_x2 );
      OUT_RELOC( dst_buffer, DRM_MM_TT|DRM_MM_WRITE, dst_offset + dst_y * dst_pitch );	
      OUT_BATCH( (0 << 16) | src_x );
      OUT_BATCH( ((GLint)src_pitch&0xffff) );
      OUT_RELOC( src_buffer, DRM_MM_TT|DRM_MM_READ, src_offset + src_y * src_pitch ); 
      ADVANCE_BATCH();
   }
}


/**
 * Use blitting to clear the renderbuffers named by 'flags'.
 * Note: we can't use the ctx->DrawBuffer->_ColorDrawBufferMask field
 * since that might include software renderbuffers or renderbuffers
 * which we're clearing with triangles.
 * \param mask  bitmask of BUFFER_BIT_* values indicating buffers to clear
 */
void intelClearWithBlit(GLcontext *ctx, GLbitfield mask, GLboolean all,
                        GLint cx, GLint cy, GLint cw, GLint ch)
{
   struct intel_context *intel = intel_context( ctx );
   GLuint clear_depth;
   GLbitfield skipBuffers = 0;
   BATCH_LOCALS;

   if (INTEL_DEBUG & DEBUG_DRI)
      _mesa_printf("%s %x\n", __FUNCTION__, mask);

   /*
    * Compute values for clearing the buffers.
    */
   clear_depth = 0;
   if (mask & BUFFER_BIT_DEPTH) {
      clear_depth = (GLuint) (ctx->DrawBuffer->_DepthMax * ctx->Depth.Clear);
   }
   if (mask & BUFFER_BIT_STENCIL) {
      clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
   }

   /* If clearing both depth and stencil, skip BUFFER_BIT_STENCIL in
    * the loop below.
    */
   if ((mask & BUFFER_BIT_DEPTH) && (mask & BUFFER_BIT_STENCIL)) {
      skipBuffers = BUFFER_BIT_STENCIL;
   }

   /* XXX Move this flush/lock into the following conditional? */
   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );

   if (intel->numClipRects)
   {
      drm_clip_rect_t clear;
      int i;

      if (intel->ctx.DrawBuffer->Name == 0) {
         /* clearing a window */

         /* flip top to bottom */
         clear.x1 = cx + intel->drawX;
         clear.y1 = intel->driDrawable->y + intel->driDrawable->h - cy - ch;
         clear.x2 = clear.x1 + cw;
         clear.y2 = clear.y1 + ch;

         /* adjust for page flipping */
         if ( intel->sarea->pf_current_page == 1 ) {
            const GLuint tmp = mask;
            mask &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT);
            if ( tmp & BUFFER_BIT_FRONT_LEFT ) mask |= BUFFER_BIT_BACK_LEFT;
            if ( tmp & BUFFER_BIT_BACK_LEFT )  mask |= BUFFER_BIT_FRONT_LEFT;
         }
      }
      else {
         /* clearing FBO */
         ASSERT(intel->numClipRects == 1);
         ASSERT(intel->pClipRects == &intel->fboRect);
         clear.x1 = cx;
         clear.y1 = intel->ctx.DrawBuffer->Height - cy - ch;
         clear.x2 = clear.y1 + cw;
         clear.y2 = clear.y1 + ch;
         /* no change to mask */
      }

      for (i = 0 ; i < intel->numClipRects ; i++) 
      { 	 
	 const drm_clip_rect_t *box = &intel->pClipRects[i];	 
	 drm_clip_rect_t b;
         GLuint buf;
         GLuint clearMask = mask; /* use copy, since we modify it below */

	 if (!all) {
	    intel_intersect_cliprects(&b, &clear, box);
	 } else {
	    b = *box;
	 }

	 if (0)
	    _mesa_printf("clear %d,%d..%d,%d, mask %x\n", 
			 b.x1, b.y1,
			 b.x2, b.y2, 
			 mask);

         /* Loop over all renderbuffers */
         for (buf = 0; buf < BUFFER_COUNT && clearMask; buf++) {
            const GLbitfield bufBit = 1 << buf;
            if ((clearMask & bufBit) && !(bufBit & skipBuffers)) {
               /* OK, clear this renderbuffer */
               const struct intel_renderbuffer *irb
                  = intel_renderbuffer(ctx->DrawBuffer->
                                       Attachment[buf].Renderbuffer);
               GLuint clearVal;
               GLint pitch, cpp;
               GLuint BR13, CMD;

               ASSERT(irb);
               ASSERT(irb->region);

               pitch = irb->region->pitch;
               cpp = irb->region->cpp;

               /* Setup the blit command */
               if (cpp == 4) {
                  BR13 = (0xF0 << 16) | (pitch * cpp) | (1<<24) | (1<<25);
                  if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL) {
                     CMD = XY_COLOR_BLT_CMD;
                     if (clearMask & BUFFER_BIT_DEPTH)
                        CMD |= XY_COLOR_BLT_WRITE_RGB;
                     if (clearMask & BUFFER_BIT_STENCIL)
                        CMD |= XY_COLOR_BLT_WRITE_ALPHA;
                  }
                  else {
                     /* clearing RGBA */
                     CMD = (XY_COLOR_BLT_CMD |
                            XY_COLOR_BLT_WRITE_ALPHA | 
                            XY_COLOR_BLT_WRITE_RGB);
                  }
               }
               else {
                  ASSERT(cpp == 2 || cpp == 0);
                  BR13 = (0xF0 << 16) | (pitch * cpp) | (1<<24);
                  CMD = XY_COLOR_BLT_CMD;
               }

               if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL) {
                  clearVal = clear_depth;
               }
               else {
                  clearVal = (cpp == 4)
                     ? intel->ClearColor8888 : intel->ClearColor565;
               }
               /*
               _mesa_debug(ctx, "hardware blit clear buf %d rb id %d\n",
                           buf, irb->Base.Name);
               */
               BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
               OUT_BATCH( CMD );
               OUT_BATCH( BR13 );
               OUT_BATCH( (b.y1 << 16) | b.x1 );
               OUT_BATCH( (b.y2 << 16) | b.x2 );
               OUT_RELOC( irb->region->buffer, DRM_MM_TT|DRM_MM_WRITE,
                          irb->region->draw_offset );
               OUT_BATCH( clearVal );
               ADVANCE_BATCH();
               clearMask &= ~bufBit; /* turn off bit, for faster loop exit */
            }
         }
      }
      intel_batchbuffer_flush( intel->batch );
   }

   UNLOCK_HARDWARE( intel );
}


