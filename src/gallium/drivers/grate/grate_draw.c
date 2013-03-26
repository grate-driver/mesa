#include <stdio.h>

#include "pipe/p_state.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_draw.h"


static void
grate_draw_vbo(struct pipe_context *pcontext,
               const struct pipe_draw_info *info)
{
   unimplemented();
}

void
grate_context_draw_init(struct pipe_context *pcontext)
{
   pcontext->draw_vbo = grate_draw_vbo;
}
