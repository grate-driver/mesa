#include <errno.h>
#include <stdio.h>

#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_draw.h"
#include "tegra_resource.h"
#include "tegra_screen.h"
#include "tegra_state.h"
#include "tegra_surface.h"

static void
tegra_context_destroy(struct pipe_context *pcontext)
{
   struct tegra_context *context = tegra_context(pcontext);

   slab_destroy_child(&context->transfer_pool);

   FREE(context);
}

static void
tegra_context_flush(struct pipe_context *pcontext,
                    struct pipe_fence_handle **pfence,
                    enum pipe_flush_flags flags)
{
   unimplemented();
}

struct pipe_context *
tegra_screen_context_create(struct pipe_screen *pscreen,
                            void *priv, unsigned flags)
{
   struct tegra_screen *screen = tegra_screen(pscreen);

   struct tegra_context *context = CALLOC_STRUCT(tegra_context);
   if (!context)
      return NULL;

   context->base.screen = pscreen;
   context->base.priv = priv;

   slab_create_child(&context->transfer_pool, &screen->transfer_pool);

   context->base.destroy = tegra_context_destroy;
   context->base.flush = tegra_context_flush;
   context->base.stream_uploader = u_upload_create_default(&context->base);
   context->base.const_uploader = context->base.stream_uploader;

   tegra_context_resource_init(&context->base);
   tegra_context_surface_init(&context->base);
   tegra_context_state_init(&context->base);
   tegra_context_blend_init(&context->base);
   tegra_context_sampler_init(&context->base);
   tegra_context_rasterizer_init(&context->base);
   tegra_context_zsa_init(&context->base);
   tegra_context_vbo_init(&context->base);
   tegra_context_draw_init(&context->base);

   return &context->base;
}
