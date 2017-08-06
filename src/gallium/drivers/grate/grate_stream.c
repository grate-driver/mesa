/*
 * Copyright (c) 2016-2017 Dmitry Osipenko <digetx@gmail.com>
 * Copyright (C) 2012-2013 NVIDIA Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS\n", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Arto Merilainen <amerilainen@nvidia.com>
 */

#include <linux/errno.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "host1x01_hardware.h"
#include "hw_host1x01_uclass.h"
#include "grate_stream.h"

#define ErrorMsg(fmt, args...) \
    fprintf(stderr, "%s:%d/%s(): " fmt, \
            __FILE__, __LINE__, __func__, ##args)

/*
 * tegra_stream_create(channel)
 *
 * Create a stream for given channel. This function preallocates several
 * command buffers for later usage to improve performance. Streams are
 * used for generating command buffers opcode by opcode using
 * tegra_stream_push().
 */

int
tegra_stream_create(struct drm_tegra *drm,
                    struct drm_tegra_channel *channel,
                    struct tegra_stream *stream,
                    uint32_t words_num)
{
   stream->status    = TEGRADRM_STREAM_FREE;
   stream->channel   = channel;
   stream->num_words = words_num;

   return 0;
}

/*
 * tegra_stream_destroy(stream)
 *
 * Destroy the given stream object. All resrouces are released.
 */

void
tegra_stream_destroy(struct tegra_stream *stream)
{
   if (!stream)
      return;

   drm_tegra_job_free(stream->job);
}

/*
 * tegra_stream_flush(stream, fence)
 *
 * Send the current contents of stream buffer. The stream must be
 * synchronized correctly (we cannot send partial streams). If
 * pointer to fence is given, the fence will contain the syncpoint value
 * that is reached when operations in the buffer are finished.
 */

int
tegra_stream_flush(struct tegra_stream *stream)
{
   struct drm_tegra_fence *fence;
   int result = 0;

   if (!stream)
      return -1;

   /* Reflushing is fine */
   if (stream->status == TEGRADRM_STREAM_FREE)
      return 0;

   /* Return error if stream is constructed badly */
   if (stream->status != TEGRADRM_STREAM_READY) {
      result = -1;
      goto cleanup;
   }

   result = drm_tegra_job_submit(stream->job, &fence);
   if (result != 0) {
      ErrorMsg("drm_tegra_job_submit() failed %d\n", result);
      result = -1;
      goto cleanup;
   }

   result = drm_tegra_fence_wait_timeout(fence, 1000);
   if (result != 0) {
      ErrorMsg("drm_tegra_fence_wait_timeout() failed %d\n", result);
      result = -1;
   }

   drm_tegra_fence_free(fence);

cleanup:
   drm_tegra_job_free(stream->job);

   stream->job = NULL;
   stream->status = TEGRADRM_STREAM_FREE;

   return result;
}

/*
 * tegra_stream_begin(stream, num_words, fence, num_fences, num_syncpt_incrs,
 *          num_relocs, class_id)
 *
 * Start constructing a stream.
 *  - num_words refer to the maximum number of words the stream can contain.
 *  - fence is a pointer to a table that contains syncpoint preconditions
 *    before the stream execution can start.
 *  - num_fences indicate the number of elements in the fence table.
 *  - num_relocs indicate the number of memory references in the buffer.
 *  - class_id refers to the class_id that is selected in the beginning of a
 *    stream. If no class id is given, the default class id (=usually the
 *    client device's class) is selected.
 *
 * This function verifies that the current buffer has enough room for holding
 * the whole stream (this is computed using num_words and num_relocs). The
 * function blocks until the stream buffer is ready for use.
 */

int
tegra_stream_begin(struct tegra_stream *stream)
{
   int ret;

   /* check stream and its state */
   if (!(stream && stream->status == TEGRADRM_STREAM_FREE)) {
      ErrorMsg("Stream status isn't FREE\n");
      return -1;
   }

   ret = drm_tegra_job_new(&stream->job, stream->channel);
   if (ret != 0) {
      ErrorMsg("drm_tegra_job_new() failed %d\n", ret);
      return -1;
   }

   ret = drm_tegra_pushbuf_new(&stream->buffer.pushbuf, stream->job);
   if (ret != 0) {
      ErrorMsg("drm_tegra_pushbuf_new() failed %d\n", ret);
      drm_tegra_job_free(stream->job);
      return -1;
   }

   ret = drm_tegra_pushbuf_prepare(stream->buffer.pushbuf, stream->num_words);
   if (ret != 0) {
      ErrorMsg("drm_tegra_pushbuf_prepare() failed %d\n", ret);
      drm_tegra_job_free(stream->job);
      return -1;
   }

   stream->class_id = 0;
   stream->status = TEGRADRM_STREAM_CONSTRUCT;

   return 0;
}

/*
 * tegra_stream_push_reloc(stream, h, offset)
 *
 * Push a memory reference to the stream.
 */

int
tegra_stream_push_reloc(struct tegra_stream *stream,
                        struct drm_tegra_bo *bo,
                        unsigned offset)
{
   int ret;

   if (!(stream && stream->status == TEGRADRM_STREAM_CONSTRUCT)) {
      ErrorMsg("Stream status isn't CONSTRUCT\n");
      return -1;
   }

   ret = drm_tegra_pushbuf_relocate(stream->buffer.pushbuf,
                                    bo, offset, 0);
   if (ret != 0) {
      stream->status = TEGRADRM_STREAM_CONSTRUCTION_FAILED;
      ErrorMsg("drm_tegra_pushbuf_relocate() failed %d\n", ret);
      return -1;
   }

   return 0;
}

/*
 * tegra_stream_push(stream, word)
 *
 * Push a single word to given stream.
 */

int
tegra_stream_push(struct tegra_stream *stream, uint32_t word)
{
   int ret;

   if (!(stream && stream->status == TEGRADRM_STREAM_CONSTRUCT)) {
      ErrorMsg("Stream status isn't CONSTRUCT\n");
      return -1;
   }

   ret = drm_tegra_pushbuf_prepare(stream->buffer.pushbuf, 1);
   if (ret != 0) {
      stream->status = TEGRADRM_STREAM_CONSTRUCTION_FAILED;
      ErrorMsg("drm_tegra_pushbuf_prepare() failed %d\n", ret);
      return -1;
   }

   *stream->buffer.pushbuf->ptr++ = word;

   return 0;
}

/*
 * tegra_stream_push_setclass(stream, class_id)
 *
 * Push "set class" opcode to the stream. Do nothing if the class is already
 * active
 */

int
tegra_stream_push_setclass(struct tegra_stream *stream, unsigned class_id)
{
   int result;

   if (stream->class_id == class_id)
      return 0;

   result = tegra_stream_push(stream, host1x_opcode_setclass(class_id, 0, 0));

   if (result == 0)
      stream->class_id = class_id;

   return result;
}

/*
 * tegra_stream_end(stream)
 *
 * Mark end of stream. This function pushes last syncpoint increment for
 * marking end of stream.
 */

int
tegra_stream_end(struct tegra_stream *stream)
{
   int ret;

   if (!(stream && stream->status == TEGRADRM_STREAM_CONSTRUCT)) {
      ErrorMsg("Stream status isn't CONSTRUCT\n");
      return -1;
   }

   ret = drm_tegra_pushbuf_sync(stream->buffer.pushbuf,
                                DRM_TEGRA_SYNCPT_COND_OP_DONE);
   if (ret != 0) {
      stream->status = TEGRADRM_STREAM_CONSTRUCTION_FAILED;
      ErrorMsg("drm_tegra_pushbuf_sync() failed %d\n", ret);
      return -1;
   }

   stream->status = TEGRADRM_STREAM_READY;

   return 0;
}

/*
 * tegra_reloc (variable, handle, offset)
 *
 * This function creates a reloc allocation. The function should be used in
 * conjunction with tegra_stream_push_words.
 */

struct tegra_reloc
tegra_reloc(const void *var_ptr, struct drm_tegra_bo *bo,
            uint32_t offset, uint32_t var_offset)
{
   struct tegra_reloc reloc = {var_ptr, bo, offset, var_offset};
   return reloc;
}

/*
 * tegra_stream_push_words(stream, addr, words, ...)
 *
 * Push words from given address to stream. The function takes
 * reloc structs as its argument. You can generate the structs with tegra_reloc
 * function.
 */

int tegra_stream_push_words(struct tegra_stream *stream, const void *addr,
                            unsigned words, int num_relocs, ...)
{
   struct tegra_reloc reloc_arg;
   va_list ap;
   uint32_t *pushbuf_ptr;
   int ret;

   if (!(stream && stream->status == TEGRADRM_STREAM_CONSTRUCT)) {
      ErrorMsg("Stream status isn't CONSTRUCT\n");
      return -1;
   }

   ret = drm_tegra_pushbuf_prepare(stream->buffer.pushbuf, words);
   if (ret != 0) {
      stream->status = TEGRADRM_STREAM_CONSTRUCTION_FAILED;
      ErrorMsg("drm_tegra_pushbuf_prepare() failed %d\n", ret);
      return -1;
   }

   /* Class id should be set explicitly, for simplicity. */
   if (stream->class_id == 0) {
      stream->status = TEGRADRM_STREAM_CONSTRUCTION_FAILED;
      ErrorMsg("HOST1X class not specified\n");
      return -1;
   }

   /* Copy the contents */
   pushbuf_ptr = stream->buffer.pushbuf->ptr;
   memcpy(pushbuf_ptr, addr, words * sizeof(uint32_t));

   /* Copy relocs */
   va_start(ap, num_relocs);
   for (; num_relocs; num_relocs--) {
      reloc_arg = va_arg(ap, struct tegra_reloc);

      stream->buffer.pushbuf->ptr  = pushbuf_ptr;
      stream->buffer.pushbuf->ptr += reloc_arg.var_offset / sizeof(uint32_t);

      ret = drm_tegra_pushbuf_relocate(stream->buffer.pushbuf, reloc_arg.bo,
                                       reloc_arg.offset, 0);
      if (ret != 0) {
         stream->status = TEGRADRM_STREAM_CONSTRUCTION_FAILED;
         ErrorMsg("drm_tegra_pushbuf_relocate() failed %d\n", ret);
         break;
      }
   }
   va_end(ap);

   stream->buffer.pushbuf->ptr = pushbuf_ptr + words;

   return ret ? -1 : 0;
}
