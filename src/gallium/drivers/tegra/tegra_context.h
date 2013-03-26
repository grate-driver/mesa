#ifndef TEGRA_CONTEXT_H
#define TEGRA_CONTEXT_H

#include "pipe/p_context.h"

struct tegra_context {
	struct pipe_context base;
};

static inline struct tegra_context *tegra_context(struct pipe_context *context)
{
	return (struct tegra_context *)context;
}

struct pipe_context *tegra_screen_context_create(struct pipe_screen *pscreen,
						 void *priv, unsigned flags);

#endif
