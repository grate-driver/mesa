#include <errno.h>
#include <stdio.h>

#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_draw.h"
#include "grate_resource.h"
#include "grate_screen.h"
#include "grate_state.h"
#include "grate_surface.h"

static void
grate_context_destroy(struct pipe_context *pcontext)
{
   struct grate_context *context = grate_context(pcontext);

   slab_destroy_child(&context->transfer_pool);

   FREE(context);
}

static void
grate_context_flush(struct pipe_context *pcontext,
                    struct pipe_fence_handle **pfence,
                    enum pipe_flush_flags flags)
{
   unimplemented();
}

struct pipe_context *
grate_screen_context_create(struct pipe_screen *pscreen,
                            void *priv, unsigned flags)
{
   struct grate_screen *screen = grate_screen(pscreen);

   struct grate_context *context = CALLOC_STRUCT(grate_context);
   if (!context)
      return NULL;

   context->base.screen = pscreen;
   context->base.priv = priv;

   slab_create_child(&context->transfer_pool, &screen->transfer_pool);

   context->base.destroy = grate_context_destroy;
   context->base.flush = grate_context_flush;
   context->base.stream_uploader = u_upload_create_default(&context->base);
   context->base.const_uploader = context->base.stream_uploader;

   grate_context_resource_init(&context->base);
   grate_context_surface_init(&context->base);
   grate_context_state_init(&context->base);
   grate_context_blend_init(&context->base);
   grate_context_sampler_init(&context->base);
   grate_context_rasterizer_init(&context->base);
   grate_context_zsa_init(&context->base);
   grate_context_vbo_init(&context->base);
   grate_context_draw_init(&context->base);

   return &context->base;
}
