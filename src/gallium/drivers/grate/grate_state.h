#ifndef GRATE_STATE_H
#define GRATE_STATE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct grate_context;

struct grate_zsa_state {
   struct pipe_depth_stencil_alpha_state base;
   uint32_t commands[2];
};

struct grate_vertexbuf_state {
   struct pipe_vertex_buffer vb[PIPE_MAX_ATTRIBS];
   unsigned int count;
   uint32_t enabled;
};

struct grate_vertex_element {
   uint32_t attrib;
   unsigned int buffer_index;
   unsigned int offset;
};

struct grate_vertex_state {
   struct grate_vertex_element elements[PIPE_MAX_ATTRIBS];
   unsigned int num_elements;
};

void
grate_emit_state(struct grate_context *context);

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
