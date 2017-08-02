#ifndef GRATE_CONTEXT_H
#define GRATE_CONTEXT_H

#include "util/slab.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "grate_state.h"
#include "grate_stream.h"

struct grate_framebuffer_state {
   struct pipe_framebuffer_state base;
   int num_rts;
   struct drm_tegra_bo *bos[16];
   uint32_t rt_params[16];
   uint32_t mask;
};

struct grate_channel {
   struct grate_context *context;
   struct grate_stream stream;
};

struct grate_context {
   struct pipe_context base;

   struct grate_channel *gr2d;
   struct grate_channel *gr3d;

   struct grate_framebuffer_state framebuffer;

   struct slab_child_pool transfer_pool;

   struct grate_vertex_state *vs;
   struct grate_vertexbuf_state vbs;
};

static inline struct grate_context *
grate_context(struct pipe_context *context)
{
   return (struct grate_context *)context;
}

struct pipe_context *
grate_screen_context_create(struct pipe_screen *pscreen,
                            void *priv, unsigned flags);

#endif
