/*
 * Copyright © 2012, 2013 Thierry Reding
 * Copyright © 2013 Erik Faye-Lund
 * Copyright © 2014 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __DRM_OPENTEGRA_H__
#define __DRM_OPENTEGRA_H__ 1

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "opentegra_drm.h"

#ifndef __maybe_unused
#define __maybe_unused  __attribute__((unused))
#endif

enum drm_tegra_class {
	DRM_TEGRA_GR2D,
	DRM_TEGRA_GR3D,
};

enum drm_tegra_soc_id {
	DRM_TEGRA_INVALID_SOC,
	DRM_TEGRA_UNKOWN_SOC,
	DRM_TEGRA20_SOC,
	DRM_TEGRA30_SOC,
	DRM_TEGRA114_SOC,
};

static __maybe_unused const char * const drm_tegra_soc_names[] = {
	[DRM_TEGRA_INVALID_SOC] = "invalid",
	[DRM_TEGRA_UNKOWN_SOC] = "unknown",
	[DRM_TEGRA20_SOC] = "Tegra20",
	[DRM_TEGRA30_SOC] = "Tegra30",
	[DRM_TEGRA114_SOC] = "Tegra114",
};

struct drm_tegra_bo;
struct drm_tegra;

int drm_tegra_new(struct drm_tegra **drmp, int fd);
void drm_tegra_close(struct drm_tegra *drm);

int drm_tegra_version(struct drm_tegra *drm);

enum drm_tegra_soc_id drm_tegra_get_soc_id(struct drm_tegra *drm);

int drm_tegra_bo_new(struct drm_tegra_bo **bop, struct drm_tegra *drm,
		     uint32_t flags, uint32_t size);
int drm_tegra_bo_wrap(struct drm_tegra_bo **bop, struct drm_tegra *drm,
		      uint32_t handle, uint32_t flags, uint32_t size);
struct drm_tegra_bo *drm_tegra_bo_ref(struct drm_tegra_bo *bo);
int drm_tegra_bo_unref(struct drm_tegra_bo *bo);
int drm_tegra_bo_get_handle(struct drm_tegra_bo *bo, uint32_t *handle);
int drm_tegra_bo_map(struct drm_tegra_bo *bo, void **ptr);
int drm_tegra_bo_unmap(struct drm_tegra_bo *bo);

int drm_tegra_bo_get_flags(struct drm_tegra_bo *bo, uint32_t *flags);
int drm_tegra_bo_set_flags(struct drm_tegra_bo *bo, uint32_t flags);

struct drm_tegra_bo_tiling {
	uint32_t mode;
	uint32_t value;
};

int drm_tegra_bo_get_tiling(struct drm_tegra_bo *bo,
			    struct drm_tegra_bo_tiling *tiling);
int drm_tegra_bo_set_tiling(struct drm_tegra_bo *bo,
			    const struct drm_tegra_bo_tiling *tiling);

int drm_tegra_bo_get_name(struct drm_tegra_bo *bo, uint32_t *name);
int drm_tegra_bo_from_name(struct drm_tegra_bo **bop, struct drm_tegra *drm,
			   uint32_t name, uint32_t flags);

int drm_tegra_bo_to_dmabuf(struct drm_tegra_bo *bo, uint32_t *handle);
int drm_tegra_bo_from_dmabuf(struct drm_tegra_bo **bop, struct drm_tegra *drm,
			     int fd, uint32_t flags);

int drm_tegra_bo_get_size(struct drm_tegra_bo *bo, uint32_t *size);
int drm_tegra_bo_forbid_caching(struct drm_tegra_bo *bo);
int drm_tegra_bo_cpu_prep(struct drm_tegra_bo *bo,
			  uint32_t flags, uint32_t timeout_us);

struct drm_tegra_channel;
struct drm_tegra_job;

struct drm_tegra_pushbuf {
	uint32_t *ptr;
};

struct drm_tegra_fence;

enum drm_tegra_syncpt_cond {
	DRM_TEGRA_SYNCPT_COND_IMMEDIATE,
	DRM_TEGRA_SYNCPT_COND_OP_DONE,
	DRM_TEGRA_SYNCPT_COND_RD_DONE,
	DRM_TEGRA_SYNCPT_COND_WR_SAFE,
	DRM_TEGRA_SYNCPT_COND_MAX,
};

int drm_tegra_channel_open(struct drm_tegra_channel **channelp,
			   struct drm_tegra *drm,
			   enum drm_tegra_class client);
int drm_tegra_channel_close(struct drm_tegra_channel *channel);

int drm_tegra_job_new(struct drm_tegra_job **jobp,
		      struct drm_tegra_channel *channel);
int drm_tegra_job_free(struct drm_tegra_job *job);
int drm_tegra_job_submit(struct drm_tegra_job *job,
			 struct drm_tegra_fence **fencep);

int drm_tegra_pushbuf_new(struct drm_tegra_pushbuf **pushbufp,
			  struct drm_tegra_job *job);
int drm_tegra_pushbuf_free(struct drm_tegra_pushbuf *pushbuf);
int drm_tegra_pushbuf_prepare(struct drm_tegra_pushbuf *pushbuf,
			      unsigned int words);
int drm_tegra_pushbuf_relocate(struct drm_tegra_pushbuf *pushbuf,
			       struct drm_tegra_bo *target,
			       unsigned long offset,
			       unsigned long shift);
int drm_tegra_pushbuf_sync(struct drm_tegra_pushbuf *pushbuf,
			   enum drm_tegra_syncpt_cond cond);

int drm_tegra_fence_wait_timeout(struct drm_tegra_fence *fence,
				 unsigned long timeout);
void drm_tegra_fence_free(struct drm_tegra_fence *fence);

static inline int drm_tegra_fence_wait(struct drm_tegra_fence *fence)
{
	return drm_tegra_fence_wait_timeout(fence, -1);
}

struct drm_tegra_job_v2 {
	struct drm_tegra *drm;
	struct drm_tegra_bo_table_entry *bo_table;
	unsigned int num_bos;
	unsigned int num_bos_max;
	unsigned int num_words;
	uint32_t *start;
	uint32_t *ptr;
};

int drm_tegra_job_new_v2(struct drm_tegra_job_v2 **jobp,
			 struct drm_tegra *drm,
			 unsigned int num_bos_expected,
			 unsigned int num_words_expected);
int drm_tegra_job_resize_v2(struct drm_tegra_job_v2 *job,
			    unsigned int num_words,
			    unsigned int num_bos,
			    bool reallocate);
int drm_tegra_job_reset_v2(struct drm_tegra_job_v2 *job);
int drm_tegra_job_free_v2(struct drm_tegra_job_v2 *job);
int drm_tegra_job_push_reloc_v2(struct drm_tegra_job_v2 *job,
				struct drm_tegra_bo *target,
				unsigned long offset,
				uint32_t flags);
int drm_tegra_job_submit_v2(struct drm_tegra_job_v2 *job,
			    uint32_t syncobj_handle,
			    uint64_t pipes_mask);

#endif /* __DRM_TEGRA_H__ */
