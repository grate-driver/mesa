#ifndef TEGRA_RESOURCE_H
#define TEGRA_RESOURCE_H

#include "pipe/p_screen.h"
#include "util/u_transfer.h"

struct tegra_resource {
   struct u_resource base;
   struct drm_tegra_bo *bo;
   unsigned int pitch;
   unsigned int tiled : 1;
   unsigned int format : 5;
};

static inline struct tegra_resource *
tegra_resource(struct pipe_resource *resource)
{
   return (struct tegra_resource *)resource;
}

int
tegra_pixel_format(enum pipe_format format);

void
tegra_context_resource_init(struct pipe_context *pcontext);

void
tegra_screen_resource_init(struct pipe_screen *pscreen);

#endif
