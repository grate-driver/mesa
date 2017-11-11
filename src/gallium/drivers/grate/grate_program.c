#include <stdio.h>

#include "util/u_memory.h"

#include "tgsi/tgsi_dump.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_screen.h"
#include "grate_program.h"


static void *
grate_create_vs_state(struct pipe_context *pcontext,
                      const struct pipe_shader_state *template)
{
   struct grate_vertex_shader_state *so =
      CALLOC_STRUCT(grate_vertex_shader_state);

   if (!so)
      return NULL;

   so->base = *template;

   /* TODO: generate code! */

   return so;
}

static void
grate_bind_vs_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
}

static void
grate_delete_vs_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
grate_context_program_init(struct pipe_context *pcontext)
{
   pcontext->create_vs_state = grate_create_vs_state;
   pcontext->bind_vs_state = grate_bind_vs_state;
   pcontext->delete_vs_state = grate_delete_vs_state;
}
