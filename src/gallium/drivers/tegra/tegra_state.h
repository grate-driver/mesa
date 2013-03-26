#ifndef TEGRA_STATE_H
#define TEGRA_STATE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct tegra_blend_state {
	struct pipe_blend_state base;
};

struct tegra_rasterizer_state {
	struct pipe_rasterizer_state base;
};

struct tegra_zsa_state {
	struct pipe_depth_stencil_alpha_state base;
};

struct tegra_vs_state {
	struct pipe_shader_state base;
};

struct tegra_fs_state {
	struct pipe_shader_state base;
};

struct tegra_vbo_state {
	struct pipe_vertex_element elements[PIPE_MAX_ATTRIBS];
	unsigned int num_elements;
};

void tegra_context_state_init(struct pipe_context *pcontext);
void tegra_context_blend_init(struct pipe_context *pcontext);
void tegra_context_rasterizer_init(struct pipe_context *pcontext);
void tegra_context_zsa_init(struct pipe_context *pcontext);
void tegra_context_vs_init(struct pipe_context *pcontext);
void tegra_context_fs_init(struct pipe_context *pcontext);
void tegra_context_vbo_init(struct pipe_context *pcontext);

#endif
