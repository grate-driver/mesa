#ifndef TEGRA_RESOURCE_H
#define TEGRA_RESOURCE_H

#include "pipe/p_screen.h"
#include "pipe/p_state.h"

struct tegra_resource {
	struct pipe_resource base;
};

void tegra_screen_resource_init(struct pipe_screen *pscreen);

#endif
