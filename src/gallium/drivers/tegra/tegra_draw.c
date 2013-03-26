#include <stdio.h>

#include "pipe/p_state.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_draw.h"


static void
tegra_draw_vbo(struct pipe_context *pcontext,
               const struct pipe_draw_info *info)
{
   unimplemented();
}

void
tegra_context_draw_init(struct pipe_context *pcontext)
{
   pcontext->draw_vbo = tegra_draw_vbo;
}
