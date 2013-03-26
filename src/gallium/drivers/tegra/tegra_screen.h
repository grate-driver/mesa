#ifndef TEGRA_SCREEN_H
#define TEGRA_SCREEN_H

#include "pipe/p_screen.h"

#include <libdrm/tegra.h>

struct tegra_screen {
	struct pipe_screen base;
};

static inline struct tegra_screen *tegra_screen(struct pipe_screen *screen)
{
	return (struct tegra_screen *)screen;
}

struct pipe_screen *tegra_screen_create(struct drm_tegra *drm);

#endif
