#ifndef GRATE_SCREEN_H
#define GRATE_SCREEN_H

#include "pipe/p_screen.h"
#include "util/slab.h"

struct drm_tegra;

extern uint32_t grate_debug;

#define GRATE_DEBUG_UNIMPLEMENTED 0x1

struct grate_screen {
   struct pipe_screen base;

   struct drm_tegra *drm;
};

static inline struct grate_screen *
grate_screen(struct pipe_screen *screen)
{
   return (struct grate_screen *)screen;
}

struct pipe_screen *
grate_screen_create(struct drm_tegra *drm);

#endif
