#ifndef TEGRA_PROGRAM_H
#define TEGRA_PROGRAM_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct tegra_vs_state {
	struct pipe_shader_state base;
};

struct tegra_fs_state {
	struct pipe_shader_state base;
};

void tegra_context_program_init(struct pipe_context *pcontext);

#endif
