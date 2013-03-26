#include <stdio.h>

#include "tegra_context.h"
#include "tegra_state.h"
#include "tegra_surface.h"

static void tegra_context_destroy(struct pipe_context *pcontext)
{
	struct tegra_context *context = tegra_context(pcontext);

	fprintf(stdout, "> %s(pcontext=%p)\n", __func__, pcontext);

	free(context);

	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_context_flush(struct pipe_context *pcontext,
				struct pipe_fence_handle **fence,
				enum pipe_flush_flags flags)
{
	fprintf(stdout, "> %s(pcontext=%p, fence=%p, flags=%x)\n", __func__,
		pcontext, fence, flags);
	fprintf(stdout, "< %s()\n", __func__);
}

struct pipe_context *tegra_screen_context_create(struct pipe_screen *pscreen,
						 void *priv, unsigned flags)
{
	struct tegra_context *context;

	fprintf(stdout, "> %s(pscreen=%p, priv=%p)\n", __func__, pscreen, priv);

	context = calloc(1, sizeof(*context));
	if (!context) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	context->base.screen = pscreen;
	context->base.priv = priv;

	context->base.destroy = tegra_context_destroy;
	context->base.flush = tegra_context_flush;

	tegra_context_surface_init(&context->base);
	tegra_context_state_init(&context->base);
	tegra_context_blend_init(&context->base);
	tegra_context_rasterizer_init(&context->base);
	tegra_context_zsa_init(&context->base);
	tegra_context_vs_init(&context->base);
	tegra_context_fs_init(&context->base);
	tegra_context_vbo_init(&context->base);

	fprintf(stdout, "< %s() = %p\n", __func__, &context->base);
	return &context->base;
}
