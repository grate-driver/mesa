/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Steamboat Springs, CO.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

#include "intel_bufmgr.h"

#include "intel_context.h"
#include "intel_ioctl.h"

#include "hash.h"
#include "simple_list.h"
#include "mm.h"
#include "imports.h"
#include "glthread.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <drm.h>

struct _mesa_HashTable;

/* The buffer manager is really part of the gl_shared_state struct.
 * TODO: Organize for the bufmgr to be created/deleted with the shared
 * state and stored within the DriverData of that struct.  Currently
 * there are no mesa callbacks for this.
 */

#define BM_MAX 16
static struct bufmgr
{
   _glthread_Mutex mutex;	/**< for thread safety */
   int driFd;
   int refcount;
   struct _mesa_HashTable *hash;

   unsigned buf_nr;			/* for generating ids */
   drmMMPool batchPool;

} bufmgr_pool[BM_MAX];

static int nr_bms;

#define LOCK(bm) _glthread_LOCK_MUTEX(bm->mutex)
#define UNLOCK(bm) _glthread_UNLOCK_MUTEX(bm->mutex)

static void
bmError(int val, const char *file, const char *function, int line)
{
   _mesa_printf("Fatal video memory manager error \"%s\".\n"
		"Check kernel logs or set the LIBGL_DEBUG\n"
		"environment variable to \"verbose\" for more info.\n"
		"Detected in file %s, line %d, function %s.\n",
		strerror(-val), file, line, function);
#ifndef NDEBUG
   exit(-1);
#else
   abort();
#endif
}

#define BM_CKFATAL(val)					       \
  do{							       \
    int tstVal = (val);					       \
    if (tstVal) 					       \
      bmError(tstVal, __FILE__, __FUNCTION__, __LINE__);       \
  } while(0);

/***********************************************************************
 * Public functions
 */

/* The initialization functions are skewed in the fake implementation.
 * This call would be to attach to an existing manager, rather than to
 * create a local one.
 */

struct bufmgr *
bm_intel_Attach(struct intel_context *intel)
{
   GLuint i;

   for (i = 0; i < nr_bms; i++)
      if (bufmgr_pool[i].driFd == intel->driFd) {
	 bufmgr_pool[i].refcount++;
	 _mesa_printf("retrieive old bufmgr for fd %d\n",
		      bufmgr_pool[i].driFd);
	 return &bufmgr_pool[i];
      }

   if (nr_bms < BM_MAX) {
      struct bufmgr *bm = &bufmgr_pool[nr_bms++];

      _mesa_printf("create new bufmgr for fd %d\n", intel->driFd);
      bm->driFd = intel->driFd;
      bm->hash = _mesa_NewHashTable();
      bm->refcount = 1;
      _glthread_INIT_MUTEX(bm->mutex);

      drmGetLock(bm->driFd, intel->hHWContext, 0);
      BM_CKFATAL(drmMMAllocBufferPool(bm->driFd, mmPoolRing, 0,
				      DRM_MM_TT | DRM_MM_NO_EVICT |
				      DRM_MM_READ | DRM_MM_EXE |
				      BM_BATCHBUFFER, 1024 * 1024, 4096,
				      &bm->batchPool));

      drmUnlock(bm->driFd, intel->hHWContext);
      return bm;
   }

   _mesa_printf("failed to create new bufmgr for fd %d\n", intel->driFd);
   return NULL;
}

void
bmGenBuffers(struct bufmgr *bm, unsigned n, unsigned *buffers, unsigned flags)
{
   LOCK(bm);
   {
      unsigned i;
      unsigned bFlags =
	    (flags) ? flags : DRM_MM_TT | DRM_MM_VRAM | DRM_MM_SYSTEM;

      for (i = 0; i < n; i++) {
	 drmMMBuf *buf = calloc(sizeof(*buf), 1);

	 BM_CKFATAL(drmMMInitBuffer(bm->driFd, bFlags, 12, buf));
	 buf->client_priv = ++bm->buf_nr;
	 buffers[i] = buf->client_priv;
	 _mesa_HashInsert(bm->hash, buffers[i], buf);
      }
   }
   UNLOCK(bm);
}

void
bmSetShared(struct bufmgr *bm, unsigned buffer, unsigned flags,
	    unsigned long offset, void *virtual)
{
   LOCK(bm);
   {
      drmMMBuf *buf = _mesa_HashLookup(bm->hash, buffer);

      assert(buf);

      buf->flags = DRM_MM_NO_EVICT | DRM_MM_SHARED
	    | DRM_MM_WRITE | DRM_MM_READ;
      buf->flags |= flags & DRM_MM_MEMTYPE_MASK;
      buf->offset = offset;
      buf->virtual = virtual;
      BM_CKFATAL(drmMMAllocBuffer(bm->driFd, 0, NULL, 0, buf));
   }
   UNLOCK(bm);
}

void
bmDeleteBuffers(struct bufmgr *bm, unsigned n, unsigned *buffers)
{
   LOCK(bm);
   {
      unsigned i;

      for (i = 0; i < n; i++) {
	 drmMMBuf *buf = _mesa_HashLookup(bm->hash, buffers[i]);

	 if (buf) {
	    BM_CKFATAL(drmMMFreeBuffer(bm->driFd, buf));

	    _mesa_HashRemove(bm->hash, buffers[i]);
	 }
      }
   }
   UNLOCK(bm);
}

/* If buffer size changes, free and reallocate.  Otherwise update in
 * place.
 */

void
bmBufferData(struct bufmgr *bm,
	     unsigned buffer, unsigned size, const void *data, unsigned flags)
{
   LOCK(bm);
   {
      drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

      DBG("bmBufferData %d sz 0x%x data: %p\n", buffer, size, data);

      assert(buf);
      assert(!buf->mapped);

      if (buf->flags & BM_BATCHBUFFER) {
	 BM_CKFATAL(drmMMFreeBuffer(bm->driFd, buf));
	 BM_CKFATAL(drmMMAllocBuffer
		    (bm->driFd, size, &bm->batchPool, 1, buf));
      } else if (!(buf->flags & DRM_MM_SHARED)) {

	 if (buf->block && (buf->size < size || drmBufIsBusy(bm->driFd, buf))) {
	    BM_CKFATAL(drmMMFreeBuffer(bm->driFd, buf));
	 }
	 if (!buf->block) {
	    BM_CKFATAL(drmMMAllocBuffer(bm->driFd, size, NULL, 0, buf));
	 }

      }

      if (data != NULL) {

	 memcpy(drmMMMapBuffer(bm->driFd, buf), data, size);
	 drmMMUnmapBuffer(bm->driFd, buf);

      }
   }
   UNLOCK(bm);
}

/* Update the buffer in place, in whatever space it is currently resident:
 */
void
bmBufferSubData(struct bufmgr *bm,
		unsigned buffer,
		unsigned offset, unsigned size, const void *data)
{
   LOCK(bm);
   {
      drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

      DBG("bmBufferSubdata %d offset 0x%x sz 0x%x\n", buffer, offset, size);

      assert(buf);
      drmBufWaitBusy(bm->driFd, buf);

      if (size) {
	 memcpy((unsigned char *) drmMMMapBuffer(bm->driFd, buf) + offset,
                data, size);
	 drmMMUnmapBuffer(bm->driFd, buf);
      }
   }
   UNLOCK(bm);
}

/* Extract data from the buffer:
 */
void
bmBufferGetSubData(struct bufmgr *bm,
		   unsigned buffer,
		   unsigned offset, unsigned size, void *data)
{
   LOCK(bm);
   {
      drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

      DBG("bmBufferSubdata %d offset 0x%x sz 0x%x\n", buffer, offset, size);

      assert(buf);
      drmBufWaitBusy(bm->driFd, buf);

      if (size) {
	 memcpy(data,
                (unsigned char *) drmMMMapBuffer(bm->driFd, buf) + offset,
                size);
	 drmMMUnmapBuffer(bm->driFd, buf);
      }
   }
   UNLOCK(bm);
}

/* Return a pointer to whatever space the buffer is currently resident in:
 */
void *
bmMapBuffer(struct bufmgr *bm, unsigned buffer, unsigned flags)
{
   void *retval;

   LOCK(bm);
   {
      drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

      DBG("bmMapBuffer %d\n", buffer);
      DBG("Map: Block is 0x%x\n", &buf->block);

      assert(buf);
      /* assert(!buf->mapped); */
      retval = drmMMMapBuffer(bm->driFd, buf);
   }
   UNLOCK(bm);

   return retval;
}

void
bmUnmapBuffer(struct bufmgr *bm, unsigned buffer)
{
   LOCK(bm);
   {
      drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

      if (!buf)
	 goto out;

      DBG("bmUnmapBuffer %d\n", buffer);

      drmMMUnmapBuffer(bm->driFd, buf);
   }
 out:
   UNLOCK(bm);
}

/* Build the list of buffers to validate.  Note that the buffer list
 * isn't a shared structure so we don't need mutexes when manipulating
 * it.  
 *
 * XXX: need refcounting for drmMMBuf structs so that they can't be
 * deleted while on these lists.
 */
struct _drmMMBufList *
bmNewBufferList(void)
{
   return drmMMInitListHead();
}

int
bmAddBuffer(struct bufmgr *bm,
	    struct _drmMMBufList *list,
	    unsigned buffer,
	    unsigned flags,
	    unsigned *memtype_return, unsigned long *offset_return)
{
   drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

   assert(buf);
   return drmMMBufListAdd(list, buf, 0, flags, memtype_return, offset_return);
}

void
bmFreeBufferList(struct _drmMMBufList *list)
{
   drmMMFreeBufList(list);
}

int
bmScanBufferList(struct bufmgr *bm,
		 struct _drmMMBufList *list, unsigned buffer)
{
   drmMMBuf *buf = (drmMMBuf *) _mesa_HashLookup(bm->hash, buffer);

   assert(buf);
   return drmMMScanBufList(list, buf);
}

/* To be called prior to emitting commands to hardware which reference
 * these buffers.  The buffer_usage list provides information on where
 * the buffers should be placed and whether their contents need to be
 * preserved on copying.  The offset and pool data elements are return
 * values from this function telling the driver exactly where the
 * buffers are currently located.
 */

int
bmValidateBufferList(struct bufmgr *bm,
		     struct _drmMMBufList *list, unsigned flags)
{
   BM_CKFATAL(drmMMValidateBuffers(bm->driFd, list));
   return 0;
}

/* After commands are emitted but before unlocking, this must be
 * called so that the buffer manager can correctly age the buffers.
 * The buffer manager keeps track of the list of validated buffers, so
 * already knows what to apply the fence to.
 *
 * The buffer manager knows how to emit and test fences directly
 * through the drm and without callbacks or whatever into the driver.
 */
unsigned
bmFenceBufferList(struct bufmgr *bm, struct _drmMMBufList *list)
{
   drmFence fence;

   BM_CKFATAL(drmMMFenceBuffers(bm->driFd, list));
   BM_CKFATAL(drmEmitFence(bm->driFd, 0, &fence));

   return fence.fenceSeq;
}

/* This functionality is used by the buffer manager, not really sure
 * if we need to be exposing it in this way, probably libdrm will
 * offer equivalent calls.
 *
 * For now they can stay, but will likely change/move before final:
 */
unsigned
bmSetFence(struct bufmgr *bm)
{
   drmFence dFence;

   BM_CKFATAL(drmEmitFence(bm->driFd, 0, &dFence));

   return dFence.fenceSeq;
}

int
bmTestFence(struct bufmgr *bm, unsigned fence)
{
   drmFence dFence;
   int retired;

   dFence.fenceType = 0;
   dFence.fenceSeq = fence;
   BM_CKFATAL(drmTestFence(bm->driFd, dFence, 0, &retired));
   return retired;
}

void
bmFinishFence(struct bufmgr *bm, unsigned fence)
{
   drmFence dFence;
   dFence.fenceType = 0;
   dFence.fenceSeq = fence;
   BM_CKFATAL(drmWaitFence(bm->driFd, dFence));
}
