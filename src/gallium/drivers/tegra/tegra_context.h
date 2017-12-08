#ifndef TEGRA_CONTEXT_H
#define TEGRA_CONTEXT_H

#include "util/slab.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "tegra_state.h"
#include "tegra_stream.h"

struct tegra_framebuffer_state {
   struct pipe_framebuffer_state base;
   int num_rts;
   struct drm_tegra_bo *bos[16];
   uint32_t rt_params[16];
   uint32_t mask;
};

struct tegra_channel {
   struct tegra_context *context;
   struct tegra_stream stream;
};

struct tegra_context {
   struct pipe_context base;

   struct tegra_channel *gr2d;
   struct tegra_channel *gr3d;

   struct tegra_framebuffer_state framebuffer;

   struct slab_child_pool transfer_pool;

   struct tegra_vertex_state *vs;
   struct tegra_vertexbuf_state vbs;

   uint32_t no_scissor[3];
   uint32_t viewport[7];
};

static inline struct tegra_context *
tegra_context(struct pipe_context *context)
{
   return (struct tegra_context *)context;
}

struct pipe_context *
tegra_screen_context_create(struct pipe_screen *pscreen,
                            void *priv, unsigned flags);

#endif
