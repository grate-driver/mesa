#include <stdio.h>

#include "util/u_memory.h"

#include "tgsi/tgsi_dump.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_screen.h"
#include "tegra_program.h"


static void *
tegra_create_vs_state(struct pipe_context *pcontext,
                      const struct pipe_shader_state *template)
{
   struct tegra_vertex_shader_state *so =
      CALLOC_STRUCT(tegra_vertex_shader_state);

   if (!so)
      return NULL;

   so->base = *template;

   if (tegra_debug & TEGRA_DEBUG_TGSI) {
      fprintf(stderr, "DEBUG: TGSI:\n");
      tgsi_dump(template->tokens, 0);
      fprintf(stderr, "\n");
   }

   /* TODO: generate code! */

   return so;
}

static void
tegra_bind_vs_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
}

static void
tegra_delete_vs_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
tegra_context_program_init(struct pipe_context *pcontext)
{
   pcontext->create_vs_state = tegra_create_vs_state;
   pcontext->bind_vs_state = tegra_bind_vs_state;
   pcontext->delete_vs_state = tegra_delete_vs_state;
}
