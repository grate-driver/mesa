#ifndef GRATE_SURFACE_H
#define GRATE_SURFACE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct grate_surface {
   struct pipe_surface base;
};

void
grate_context_surface_init(struct pipe_context *context);

#endif
