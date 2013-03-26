#ifndef TEGRA_RESOURCE_H
#define TEGRA_RESOURCE_H

#include "pipe/p_screen.h"
#include "util/u_transfer.h"

struct tegra_resource {
   struct u_resource base;
   struct drm_tegra_bo *bo;
   unsigned int pitch;
   unsigned int tiled : 1;
};

static inline struct tegra_resource *
tegra_resource(struct pipe_resource *resource)
{
   return (struct tegra_resource *)resource;
}

void
tegra_context_resource_init(struct pipe_context *pcontext);

void
tegra_screen_resource_init(struct pipe_screen *pscreen);

#endif
