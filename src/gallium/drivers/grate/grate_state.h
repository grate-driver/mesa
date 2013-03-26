#ifndef GRATE_STATE_H
#define GRATE_STATE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

void
grate_context_state_init(struct pipe_context *pcontext);

void
grate_context_blend_init(struct pipe_context *pcontext);

void
grate_context_sampler_init(struct pipe_context *pcontext);

void
grate_context_rasterizer_init(struct pipe_context *pcontext);

void
grate_context_zsa_init(struct pipe_context *pcontext);

void
grate_context_vbo_init(struct pipe_context *pcontext);

#endif
