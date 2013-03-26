#ifndef TEGRA_SCREEN_H
#define TEGRA_SCREEN_H

#include "pipe/p_screen.h"
#include "util/slab.h"

#include <libdrm/tegra.h>

extern uint32_t tegra_debug;

#define TEGRA_DEBUG_UNIMPLEMENTED 0x1

struct tegra_screen {
   struct pipe_screen base;

   struct drm_tegra *drm;
};

static inline struct tegra_screen *
tegra_screen(struct pipe_screen *screen)
{
   return (struct tegra_screen *)screen;
}

struct pipe_screen *
tegra_screen_create(struct drm_tegra *drm);

#endif
