#ifndef GRATE_CONTEXT_H
#define GRATE_CONTEXT_H

#include "util/slab.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "grate_state.h"

struct grate_framebuffer_state {
   struct pipe_framebuffer_state base;
};

struct grate_context {
   struct pipe_context base;

   struct grate_framebuffer_state framebuffer;

   struct slab_child_pool transfer_pool;
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
