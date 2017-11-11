#ifndef GRATE_PROGRAM_H
#define GRATE_PROGRAM_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct grate_shader_blob {
   uint32_t *commands;
   int num_commands;
};

struct grate_vertex_shader_state {
   struct pipe_shader_state base;
   struct grate_shader_blob blob;
};

void
grate_context_program_init(struct pipe_context *pcontext);

#endif
