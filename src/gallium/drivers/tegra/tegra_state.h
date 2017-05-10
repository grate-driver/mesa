#ifndef TEGRA_STATE_H
#define TEGRA_STATE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct tegra_context;

void
tegra_emit_state(struct tegra_context *context);

void
tegra_context_state_init(struct pipe_context *pcontext);

void
tegra_context_blend_init(struct pipe_context *pcontext);

void
tegra_context_sampler_init(struct pipe_context *pcontext);

void
tegra_context_rasterizer_init(struct pipe_context *pcontext);

void
tegra_context_zsa_init(struct pipe_context *pcontext);

void
tegra_context_vbo_init(struct pipe_context *pcontext);

#endif
