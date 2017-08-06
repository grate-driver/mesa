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

static int
tegra_channel_create(struct tegra_context *context,
                     enum drm_tegra_class class,
                     struct tegra_channel **channelp)
{
   struct tegra_screen *screen = tegra_screen(context->base.screen);
   int err;
   struct drm_tegra_channel *drm_channel;
   struct tegra_channel *channel;

   err = drm_tegra_channel_open(&drm_channel, screen->drm, class);
   if (err < 0)
      return err;

   channel = CALLOC_STRUCT(tegra_channel);
   if (!channel)
      return -ENOMEM;

   channel->context = context;

   err = tegra_stream_create(screen->drm, drm_channel, &channel->stream, 32768);
   if (err < 0) {
      FREE(channel);
      drm_tegra_channel_close(drm_channel);
      return err;
   }

   *channelp = channel;

   return 0;
}

static void
tegra_channel_delete(struct tegra_channel *channel)
{
   tegra_stream_destroy(&channel->stream);
   drm_tegra_channel_close(channel->stream.channel);
   FREE(channel);
}

static void
tegra_context_destroy(struct pipe_context *pcontext)
{
   struct tegra_context *context = tegra_context(pcontext);

   slab_destroy_child(&context->transfer_pool);

   tegra_channel_delete(context->gr2d);
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
   int err;

   struct tegra_context *context = CALLOC_STRUCT(tegra_context);
   if (!context)
      return NULL;

   context->base.screen = pscreen;
   context->base.priv = priv;

   err = tegra_channel_create(context, DRM_TEGRA_GR2D, &context->gr2d);
   if (err < 0) {
      fprintf(stderr, "tegra_channel_create() failed: %d\n", err);
      return NULL;
   }

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
