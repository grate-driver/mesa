#ifndef TEGRA_SURFACE_H
#define TEGRA_SURFACE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct tegra_surface {
   struct pipe_surface base;
};

void
tegra_context_surface_init(struct pipe_context *context);

#endif
