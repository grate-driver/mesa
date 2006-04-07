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

<<<<<<< intel_batchbuffer.c

#include <stdio.h>
#include <errno.h>

#include "mtypes.h"
#include "context.h"
#include "enums.h"
#include "vblank.h"

#include "intel_reg.h"
=======
>>>>>>> 1.4.2.16
#include "intel_batchbuffer.h"
#include "intel_ioctl.h"
#include "intel_bufmgr.h"

/* Relocations in kernel space:
 *    - pass dma buffer seperately
 *    - memory manager knows how to patch
 *    - pass list of dependent buffers
 *    - pass relocation list
 *
 * Either:
 *    - get back an offset for buffer to fire
 *    - memory manager knows how to fire buffer
 *
 * Really want the buffer to be AGP and pinned.
 *
 */

/* Cliprect fence: The highest fence protecting a dma buffer
 * containing explicit cliprect information.  Like the old drawable
 * lock but irq-driven.  X server must wait for this fence to expire
 * before changing cliprects [and then doing sw rendering?].  For
 * other dma buffers, the scheduler will grab current cliprect info
 * and mix into buffer.  X server must hold the lock while changing
 * cliprects???  Make per-drawable.  Need cliprects in shared memory
 * -- beats storing them with every cmd buffer in the queue.
 *
 * ==> X server must wait for this fence to expire before touching the
 * framebuffer with new cliprects.
 *
 * ==> Cliprect-dependent buffers associated with a
 * cliprect-timestamp.  All of the buffers associated with a timestamp
 * must go to hardware before any buffer with a newer timestamp.
 *
 * ==> Dma should be queued per-drawable for correct X/GL
 * synchronization.  Or can fences be used for this?
 *
 * Applies to: Blit operations, metaops, X server operations -- X
 * server automatically waits on its own dma to complete before
 * modifying cliprects ???
 */				

static void intel_dump_batchbuffer( GLuint offset,
				    GLuint *ptr,
				    GLuint count )
{
   int i;
   fprintf(stderr, "\n\n\nSTART BATCH (%d dwords):\n", count/4);
   for (i = 0; i < count/4; i += 4) 
      fprintf(stderr, "0x%x:\t0x%08x 0x%08x 0x%08x 0x%08x\n", 
	      offset + i*4, ptr[i], ptr[i+1], ptr[i+2], ptr[i+3]);
   fprintf(stderr, "END BATCH\n\n\n");
}


static void intel_batchbuffer_reset( struct intel_batchbuffer *batch )
{
   bmBufferData(batch->bm,
		batch->buffer,
		BATCH_SZ,
		NULL,
		0);
		
   if (!batch->list) 
      batch->list = bmNewBufferList();

   drmMMClearBufList(batch->list);
   batch->list_count = 0;
   batch->nr_relocs = 0;
   batch->flags = 0;

   bmAddBuffer( batch->bm,
	        batch->list,
		batch->buffer,
		DRM_MM_TT,
		NULL,
		&batch->offset[batch->list_count++]);

   batch->map = bmMapBuffer(batch->bm, batch->buffer, DRM_MM_WRITE);
   batch->ptr = batch->map;
}

/*======================================================================
 * Public functions
 */
<<<<<<< intel_batchbuffer.c

static void intel_fill_box( intelContextPtr intel,
			    GLshort x, GLshort y,
			    GLshort w, GLshort h,
			    GLubyte r, GLubyte g, GLubyte b )
{
   x += intel->drawX;
   y += intel->drawY;

   if (x >= 0 && y >= 0 &&
       x+w < intel->intelScreen->width &&
       y+h < intel->intelScreen->height)
      intelEmitFillBlitLocked( intel, 
			       intel->intelScreen->cpp,
			       intel->intelScreen->back.pitch,
			       intel->intelScreen->back.offset,
			       x, y, w, h,
			       INTEL_PACKCOLOR(intel->intelScreen->fbFormat,
					       r,g,b,0xff));
}

static void intel_draw_performance_boxes( intelContextPtr intel )
=======
struct intel_batchbuffer *intel_batchbuffer_alloc( struct intel_context *intel )
>>>>>>> 1.4.2.16
{
   struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

   batch->intel = intel;
   batch->bm = intel->bm;

   bmGenBuffers(intel->bm, 1, &batch->buffer, BM_BATCHBUFFER);
   intel_batchbuffer_reset( batch );
   return batch;
}

void intel_batchbuffer_free( struct intel_batchbuffer *batch )
{
   if (batch->map)
      bmUnmapBuffer(batch->bm, batch->buffer);
   
   free(batch);
}

/* TODO: Push this whole function into bufmgr.
 */
static void do_flush_locked( struct intel_batchbuffer *batch,
			     GLuint used,
			     GLboolean ignore_cliprects,
			     GLboolean allow_unlock)
{
   GLuint *ptr;
   GLuint i;

   bmValidateBufferList( batch->bm, 
			 batch->list,
			 DRM_MM_TT );

   /* Apply the relocations.  This nasty map indicates to me that the
    * whole task should be done internally by the memory manager, and
    * that dma buffers probably need to be pinned within agp space.
    */
   ptr = (GLuint *)bmMapBuffer(batch->bm, batch->buffer, DRM_MM_WRITE);

   
   for (i = 0; i < batch->nr_relocs; i++) {
      struct buffer_reloc *r = &batch->reloc[i];
      
      assert(r->elem < batch->list_count);
      ptr[r->offset/4] = batch->offset[r->elem] + r->delta;
   }

   if (INTEL_DEBUG & DEBUG_DMA)
      intel_dump_batchbuffer( 0, ptr, used );

   bmUnmapBuffer(batch->bm, batch->buffer);
   
   /* Fire the batch buffer, which was uploaded above:
    */
<<<<<<< intel_batchbuffer.c
   intel->vtbl.emit_state( intel );
   
   /* Make sure there is some space in this buffer:
    */
   if (intel->vertex_size * 10 * sizeof(GLuint) >= intel->batch.space) {
      intelFlushBatch(intel, GL_TRUE); 
      intel->vtbl.emit_state( intel );
   }
=======
>>>>>>> 1.4.2.16

#if 1
   intel_batch_ioctl(batch->intel, 
		     batch->offset[0],
		     used,
		     ignore_cliprects,
		     allow_unlock);
#endif
<<<<<<< intel_batchbuffer.c

   /* Emit a slot which will be filled with the inline primitive
    * command later.
    */
   BEGIN_BATCH(2);
   OUT_BATCH( 0 );

   intel->prim.start_ptr = batch_ptr;
   intel->prim.primitive = prim;
   intel->prim.flush = intel_flush_inline_primitive;
   intel->batch.contains_geometry = 1;

   OUT_BATCH( 0 );
   ADVANCE_BATCH();
}


void intelRestartInlinePrimitive( intelContextPtr intel )
{
   GLuint prim = intel->prim.primitive;

   intel_flush_inline_primitive( &intel->ctx );
   if (1) intelFlushBatch(intel, GL_TRUE); /* GL_TRUE - is critical */
   intelStartInlinePrimitive( intel, prim );
=======
   batch->last_fence = bmFenceBufferList(batch->bm, batch->list);
>>>>>>> 1.4.2.16
}



GLuint intel_batchbuffer_flush( struct intel_batchbuffer *batch )
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - batch->map;

   if (used == 0) 
      return batch->last_fence;

   /* Add the MI_BATCH_BUFFER_END.  Always add an MI_FLUSH - this is a
    * performance drain that we would like to avoid.
    */
<<<<<<< intel_batchbuffer.c
   intel->vtbl.emit_state( intel );

   if ((1+dwords)*4 >= intel->batch.space) {
      intelFlushBatch(intel, GL_TRUE); 
      intel->vtbl.emit_state( intel );
   }


   if (1) {
      int used = dwords * 4;
      int vertcount;
=======
   if (used & 4) {
      ((int *)batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *)batch->ptr)[1] = 0;
      ((int *)batch->ptr)[2] = MI_BATCH_BUFFER_END;
      used += 12;
   }
   else {
      ((int *)batch->ptr)[0] = intel->vtbl.flush_cmd() ; 
      ((int *)batch->ptr)[1] = MI_BATCH_BUFFER_END;
      used += 8;
   }
>>>>>>> 1.4.2.16

   bmUnmapBuffer(batch->bm, batch->buffer);
   batch->ptr = NULL;
   batch->map = NULL;

   /* TODO: Just pass the relocation list and dma buffer up to the
    * kernel.
    */
<<<<<<< intel_batchbuffer.c
   BEGIN_BATCH(1 + dwords);
   OUT_BATCH( _3DPRIMITIVE | 
	      primitive |
	      (dwords-1) );

   tmp = (GLuint *)batch_ptr;
   batch_ptr += dwords * 4;

   ADVANCE_BATCH();

   intel->batch.contains_geometry = 1;

 do_discard:
   return tmp;
}


static void intelWaitForFrameCompletion( intelContextPtr intel )
{
  drm_i915_sarea_t *sarea = (drm_i915_sarea_t *)intel->sarea;

   if (intel->do_irqs) {
      if (intelGetLastFrame(intel) < sarea->last_dispatch) {
	 if (!intel->irqsEmitted) {
	    while (intelGetLastFrame (intel) < sarea->last_dispatch)
	       ;
	 }
	 else {
	    UNLOCK_HARDWARE( intel ); 
	    intelWaitIrq( intel, intel->alloc.irq_emitted );	
	    LOCK_HARDWARE( intel ); 
	 }
	 intel->irqsEmitted = 10;
      }

      if (intel->irqsEmitted) {
	 intelEmitIrqLocked( intel );
	 intel->irqsEmitted--;
      }
   } 
   else {
      while (intelGetLastFrame (intel) < sarea->last_dispatch) {
	 UNLOCK_HARDWARE( intel ); 
	 if (intel->do_usleeps) 
	    DO_USLEEP( 1 );
	 LOCK_HARDWARE( intel ); 
      }
   }
}

/*
 * Copy the back buffer to the front buffer. 
 */
void intelCopyBuffer( const __DRIdrawablePrivate *dPriv ) 
{
   intelContextPtr intel;
   GLboolean   missed_target;
   int64_t ust;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (intelContextPtr) dPriv->driContextPriv->driverPrivate;

   intelFlush( &intel->ctx );
   
   LOCK_HARDWARE( intel );
   intelWaitForFrameCompletion( intel );
   UNLOCK_HARDWARE( intel );
   driWaitForVBlank( dPriv, &intel->vbl_seq, intel->vblank_flags, & missed_target );

   LOCK_HARDWARE( intel );
=======
   if (!intel->locked)
>>>>>>> 1.4.2.16
   {
<<<<<<< intel_batchbuffer.c
      const intelScreenPrivate *intelScreen = intel->intelScreen;
      const __DRIdrawablePrivate *dPriv = intel->driDrawable;
      const int nbox = dPriv->numClipRects;
      const drm_clip_rect_t *pbox = dPriv->pClipRects;
      const int cpp = intelScreen->cpp;
      const int pitch = intelScreen->front.pitch; /* in bytes */
      int i;
      GLuint CMD, BR13;
      BATCH_LOCALS;

      switch(cpp) {
      case 2: 
	 BR13 = (pitch) | (0xCC << 16) | (1<<24);
	 CMD = XY_SRC_COPY_BLT_CMD;
	 break;
      case 4:
	 BR13 = (pitch) | (0xCC << 16) | (1<<24) | (1<<25);
	 CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		XY_SRC_COPY_BLT_WRITE_RGB);
	 break;
      default:
	 BR13 = (pitch) | (0xCC << 16) | (1<<24);
	 CMD = XY_SRC_COPY_BLT_CMD;
	 break;
      }
   
      if (0) 
	 intel_draw_performance_boxes( intel );

      for (i = 0 ; i < nbox; i++, pbox++) 
      {
	 if (pbox->x1 > pbox->x2 ||
	     pbox->y1 > pbox->y2 ||
	     pbox->x2 > intelScreen->width ||
	     pbox->y2 > intelScreen->height) {
            _mesa_warning(&intel->ctx, "Bad cliprect in intelCopyBuffer()");
	    continue;
         }

	 BEGIN_BATCH( 8);
	 OUT_BATCH( CMD );
	 OUT_BATCH( BR13 );
	 OUT_BATCH( (pbox->y1 << 16) | pbox->x1 );
	 OUT_BATCH( (pbox->y2 << 16) | pbox->x2 );

	 if (intel->sarea->pf_current_page == 0) 
	    OUT_BATCH( intelScreen->front.offset );
	 else
	    OUT_BATCH( intelScreen->back.offset );			

	 OUT_BATCH( (pbox->y1 << 16) | pbox->x1 );
	 OUT_BATCH( BR13 & 0xffff );

	 if (intel->sarea->pf_current_page == 0) 
	    OUT_BATCH( intelScreen->back.offset );			
	 else
	    OUT_BATCH( intelScreen->front.offset );
=======
      assert(!(batch->flags & INTEL_BATCH_NO_CLIPRECTS));
>>>>>>> 1.4.2.16

      LOCK_HARDWARE(intel);
      do_flush_locked(batch, used, GL_FALSE, GL_TRUE);
      UNLOCK_HARDWARE(intel);
   }
<<<<<<< intel_batchbuffer.c
   intelFlushBatchLocked( intel, GL_TRUE, GL_TRUE, GL_TRUE );
   UNLOCK_HARDWARE( intel );

   intel->swap_count++;
   (*dri_interface->getUST)(&ust);
   if (missed_target) {
     intel->swap_missed_count++;
     intel->swap_missed_ust = ust -  intel->swap_ust;
   }
   
   intel->swap_ust = ust;
}




void intelEmitFillBlitLocked( intelContextPtr intel,
			      GLuint cpp,
			      GLshort dst_pitch,  /* in bytes */
			      GLuint dst_offset,
			      GLshort x, GLshort y, 
			      GLshort w, GLshort h,
			      GLuint color )
{
   GLuint BR13, CMD;
   BATCH_LOCALS;

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
=======
   else {
      GLboolean ignore_cliprects = !(batch->flags & INTEL_BATCH_CLIPRECTS);
      do_flush_locked(batch, used, ignore_cliprects, GL_FALSE);
>>>>>>> 1.4.2.16
   }

   /* Reset the buffer:
    */
   intel_batchbuffer_reset( batch );
   return batch->last_fence;
}

void intel_batchbuffer_finish( struct intel_batchbuffer *batch )
{   
   bmFinishFence(batch->bm, 
		 intel_batchbuffer_flush(batch));
}
<<<<<<< intel_batchbuffer.c



void intelClearWithBlit(GLcontext *ctx, GLbitfield flags, GLboolean all,
		      GLint cx1, GLint cy1, GLint cw, GLint ch)
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   intelScreenPrivate *intelScreen = intel->intelScreen;
   GLuint clear_depth, clear_color;
   GLint cx, cy;
   GLint pitch;
   GLint cpp = intelScreen->cpp;
   GLint i;
   GLuint BR13, CMD, D_CMD;
   BATCH_LOCALS;

   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );

   pitch = intelScreen->front.pitch;

   clear_color = intel->ClearColor;
   clear_depth = 0;
=======
   
>>>>>>> 1.4.2.16

/*  This is the only way buffers get added to the validate list.
 */
GLboolean intel_batchbuffer_emit_reloc( struct intel_batchbuffer *batch,
					GLuint buffer,
					GLuint flags,
					GLuint delta )
{
   GLuint i;

   assert(batch->nr_relocs <= MAX_RELOCS);

   i = bmScanBufferList(batch->bm, batch->list, buffer);
   if (i == -1) {
      i = batch->list_count; 
      bmAddBuffer(batch->bm,
		  batch->list,
		  buffer,
		  flags,
		  NULL,
		  &batch->offset[batch->list_count++]);
   }

<<<<<<< intel_batchbuffer.c
   if (flags & BUFFER_BIT_STENCIL) {
      clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
   }

   switch(cpp) {
   case 2: 
      BR13 = (0xF0 << 16) | (pitch) | (1<<24);
      D_CMD = CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = (0xF0 << 16) | (pitch) | (1<<24) | (1<<25);
      CMD = (XY_COLOR_BLT_CMD |
	     XY_COLOR_BLT_WRITE_ALPHA | 
	     XY_COLOR_BLT_WRITE_RGB);
      D_CMD = XY_COLOR_BLT_CMD;
      if (flags & BUFFER_BIT_DEPTH) D_CMD |= XY_COLOR_BLT_WRITE_RGB;
      if (flags & BUFFER_BIT_STENCIL) D_CMD |= XY_COLOR_BLT_WRITE_ALPHA;
      break;
   default:
      BR13 = (0xF0 << 16) | (pitch) | (1<<24);
      D_CMD = CMD = XY_COLOR_BLT_CMD;
      break;
=======
   {
      struct buffer_reloc *r = &batch->reloc[batch->nr_relocs++];
      r->offset = batch->ptr - batch->map;
      r->delta = delta;
      r->elem = i;
>>>>>>> 1.4.2.16
   }

<<<<<<< intel_batchbuffer.c
   {
      /* flip top to bottom */
      cy = intel->driDrawable->h-cy1-ch;
      cx = cx1 + intel->drawX;
      cy += intel->drawY;

      /* adjust for page flipping */
      if ( intel->sarea->pf_current_page == 1 ) {
	 GLuint tmp = flags;

	 flags &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT);
	 if ( tmp & BUFFER_BIT_FRONT_LEFT ) flags |= BUFFER_BIT_BACK_LEFT;
	 if ( tmp & BUFFER_BIT_BACK_LEFT )  flags |= BUFFER_BIT_FRONT_LEFT;
      }

      for (i = 0 ; i < intel->numClipRects ; i++) 
      { 	 
	 drm_clip_rect_t *box = &intel->pClipRects[i];	 
	 drm_clip_rect_t b;

	 if (!all) {
	    GLint x = box[i].x1;
	    GLint y = box[i].y1;
	    GLint w = box[i].x2 - x;
	    GLint h = box[i].y2 - y;

	    if (x < cx) w -= cx - x, x = cx; 
	    if (y < cy) h -= cy - y, y = cy;
	    if (x + w > cx + cw) w = cx + cw - x;
	    if (y + h > cy + ch) h = cy + ch - y;
	    if (w <= 0) continue;
	    if (h <= 0) continue;

	    b.x1 = x;
	    b.y1 = y;
	    b.x2 = x + w;
	    b.y2 = y + h;      
	 } else {
	    b = *box;
	 }


	 if (b.x1 > b.x2 ||
	     b.y1 > b.y2 ||
	     b.x2 > intelScreen->width ||
	     b.y2 > intelScreen->height)
	    continue;

	 if ( flags & BUFFER_BIT_FRONT_LEFT ) {	    
	    BEGIN_BATCH( 6);	    
	    OUT_BATCH( CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( intelScreen->front.offset );
	    OUT_BATCH( clear_color );
	    ADVANCE_BATCH();
	 }

	 if ( flags & BUFFER_BIT_BACK_LEFT ) {
	    BEGIN_BATCH( 6); 
	    OUT_BATCH( CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( intelScreen->back.offset );
	    OUT_BATCH( clear_color );
	    ADVANCE_BATCH();
	 }

	 if ( flags & (BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH) ) {
	    BEGIN_BATCH( 6);
	    OUT_BATCH( D_CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( intelScreen->depth.offset );
	    OUT_BATCH( clear_depth );
	    ADVANCE_BATCH();
	 }      
      }
   }
   intelFlushBatchLocked( intel, GL_TRUE, GL_FALSE, GL_TRUE );
   UNLOCK_HARDWARE( intel );
=======
   batch->ptr += 4;
   return GL_TRUE;
>>>>>>> 1.4.2.16
}



void intel_batchbuffer_data(struct intel_batchbuffer *batch,
			    const void *data,
			    GLuint bytes,
			    GLuint flags)
{
<<<<<<< intel_batchbuffer.c
   intelContextPtr intel = INTEL_CONTEXT(ctx);

   if (intel->alloc.offset) {
      intelFreeAGP( intel, intel->alloc.ptr );
      intel->alloc.ptr = NULL;
      intel->alloc.offset = 0;
   }
   else if (intel->alloc.ptr) {
      free(intel->alloc.ptr);
      intel->alloc.ptr = NULL;
   }

   memset(&intel->batch, 0, sizeof(intel->batch));
=======
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(batch, bytes, flags);
   __memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
>>>>>>> 1.4.2.16
}

<<<<<<< intel_batchbuffer.c

void intelInitBatchBuffer( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);

   /* This path isn't really safe with rotate:
    */
   if (getenv("INTEL_BATCH") && intel->intelScreen->allow_batchbuffer) {      
      switch (intel->intelScreen->deviceID) {
      case PCI_CHIP_I865_G:
	 /* HW bug?  Seems to crash if batchbuffer crosses 4k boundary.
	  */
	 intel->alloc.size = 8 * 1024; 
	 break;
      default:
	 /* This is the smallest amount of memory the kernel deals with.
	  * We'd ideally like to make this smaller.
	  */
	 intel->alloc.size = 1 << intel->intelScreen->logTextureGranularity;
	 break;
      }

      /* KW: temporary - this make crashes & lockups more frequent, so
       * leave in until they are solved.
       */
      intel->alloc.size = 8 * 1024; 

      intel->alloc.ptr = intelAllocateAGP( intel, intel->alloc.size );
      if (intel->alloc.ptr)
	 intel->alloc.offset = 
	    intelAgpOffsetFromVirtual( intel, intel->alloc.ptr );
      else
         intel->alloc.offset = 0; /* OK? */
   }

   /* The default is now to use a local buffer and pass that to the
    * kernel.  This is also a fallback if allocation fails on the
    * above path:
    */
   if (!intel->alloc.ptr) {
      intel->alloc.size = 8 * 1024;
      intel->alloc.ptr = malloc( intel->alloc.size );
      intel->alloc.offset = 0;
   }

   assert(intel->alloc.ptr);
}
=======
>>>>>>> 1.4.2.16
