#include <errno.h>
#include <stdio.h>

#include "util/u_inlines.h"

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
	struct tegra_channel *channel;
	int err;

	fprintf(stdout, "> %s(context=%p, client=%d, channelp=%p)\n",
		__func__, context, client, channelp);

	channel = calloc(1, sizeof(*channel));
	if (!channel)
		return -ENOMEM;

	channel->context = context;

	err = tegra_stream_create(screen->drm, &channel->stream, 32768);
	if (err < 0) {
		free(channel);
		return err;
	}

	*channelp = channel;

	fprintf(stdout, "< %s()\n", __func__);
	return 0;
}

static void tegra_channel_delete(struct tegra_channel *channel)
{
	fprintf(stdout, "> %s(channel=%p)\n", __func__, channel);

	tegra_stream_destroy(&channel->stream);
	free(channel);

	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_channel_flush(struct tegra_channel *channel)
{
	int err;

	fprintf(stdout, "> %s(channel=%p)\n", __func__, channel);

	err = tegra_stream_flush(&channel->stream);
	if (err < 0) {
		fprintf(stderr, "tegra_stream_flush() failed: %d\n", err);
		goto out;
	}

out:
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_context_destroy(struct pipe_context *pcontext)
{
	struct tegra_context *context = tegra_context(pcontext);

	fprintf(stdout, "> %s(pcontext=%p)\n", __func__, pcontext);

	tegra_channel_delete(context->gr3d);
	tegra_channel_delete(context->gr2d);
	free(context);

	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_context_flush(struct pipe_context *pcontext,
				struct pipe_fence_handle **pfence,
				enum pipe_flush_flags flags)
{
	struct tegra_context *context = tegra_context(pcontext);

	fprintf(stdout, "> %s(pcontext=%p, pfence=%p, flags=%x)\n", __func__,
		pcontext, pfence, flags);

	tegra_channel_flush(context->gr2d);
	tegra_channel_flush(context->gr3d);

	if (pfence) {
		struct tegra_fence *fence;

		fence = calloc(1, sizeof(*fence));
		if (!fence)
			goto out;

		pipe_reference_init(&fence->reference, 1);

		*pfence = (struct pipe_fence_handle *)fence;
	}

out:
	fprintf(stdout, "< %s()\n", __func__);
}

struct pipe_context *tegra_screen_context_create(struct pipe_screen *pscreen,
						 void *priv, unsigned flags)
{
	struct tegra_context *context;
	int err;

	fprintf(stdout, "> %s(pscreen=%p, priv=%p)\n", __func__, pscreen, priv);

	context = calloc(1, sizeof(*context));
	if (!context) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

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

	context->base.destroy = tegra_context_destroy;
	context->base.flush = tegra_context_flush;

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

	fprintf(stdout, "< %s() = %p\n", __func__, &context->base);
	return &context->base;
}
