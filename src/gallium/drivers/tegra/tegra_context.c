#include <errno.h>
#include <stdio.h>

#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

#include "tegra_context.h"
#include "tegra_resource.h"
#include "tegra_screen.h"
#include "tegra_state.h"
#include "tegra_surface.h"

static int tegra_channel_create(struct tegra_context *context,
				enum host1x_class client,
				struct tegra_channel **channelp)
{
	struct tegra_screen *screen = tegra_screen(context->base.screen);
	int err;

	struct tegra_channel *channel = CALLOC_STRUCT(tegra_channel);
	if (!channel)
		return -ENOMEM;

	channel->context = context;

	err = tegra_stream_create(screen->drm, &channel->stream, 32768);
	if (err < 0) {
		FREE(channel);
		return err;
	}

	*channelp = channel;

	return 0;
}

static void tegra_channel_delete(struct tegra_channel *channel)
{
	tegra_stream_destroy(&channel->stream);
	FREE(channel);
}

static void tegra_channel_flush(struct tegra_channel *channel)
{
	int err;

	err = tegra_stream_flush(&channel->stream);
	if (err < 0)
		fprintf(stderr, "tegra_stream_flush() failed: %d\n", err);
}

static void tegra_context_destroy(struct pipe_context *pcontext)
{
	struct tegra_context *context = tegra_context(pcontext);

	slab_destroy_child(&context->transfer_pool);

	tegra_channel_delete(context->gr3d);
	tegra_channel_delete(context->gr2d);
	FREE(context);
}

static void tegra_context_flush(struct pipe_context *pcontext,
				struct pipe_fence_handle **pfence,
				enum pipe_flush_flags flags)
{
	struct tegra_context *context = tegra_context(pcontext);

	tegra_channel_flush(context->gr2d);
	tegra_channel_flush(context->gr3d);

	if (pfence) {
		struct tegra_fence *fence;

		fence = CALLOC_STRUCT(tegra_fence);
		if (!fence) {
			perror("calloc failed"); /* TODO: need a better way of handling this */
			return;
		}

		pipe_reference_init(&fence->reference, 1);

		*pfence = (struct pipe_fence_handle *)fence;
	}
}

struct pipe_context *tegra_screen_context_create(struct pipe_screen *pscreen,
						 void *priv, unsigned flags)
{
	struct tegra_screen *screen = tegra_screen(pscreen);
	int err;

	struct tegra_context *context = CALLOC_STRUCT(tegra_context);
	if (!context)
		return NULL;

	context->base.screen = pscreen;
	context->base.priv = priv;

	err = tegra_channel_create(context, HOST1X_CLASS_GR2D, &context->gr2d);
	if (err < 0) {
		fprintf(stderr, "tegra_channel_create() failed: %d\n", err);
		return NULL;
	}

	err = tegra_channel_create(context, HOST1X_CLASS_GR3D, &context->gr3d);
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
	tegra_context_vs_init(&context->base);
	tegra_context_fs_init(&context->base);
	tegra_context_vbo_init(&context->base);

	return &context->base;
}
