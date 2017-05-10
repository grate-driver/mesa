#include <stdio.h>

#include "pipe/p_state.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_draw.h"
#include "grate_state.h"


static void
grate_draw_vbo(struct pipe_context *pcontext,
               const struct pipe_draw_info *info)
{
   int err;
   struct grate_context *context = grate_context(pcontext);
   struct grate_stream *stream = &context->gr3d->stream;

   err = grate_stream_begin(stream);
   if (err < 0) {
      fprintf(stderr, "grate_stream_begin() failed: %d\n", err);
      return;
   }

   grate_stream_push_setclass(stream, HOST1X_CLASS_GR3D);

   grate_emit_state(context);

   /* TODO: draw */

   grate_stream_end(stream);

   grate_stream_flush(stream);
}

void
grate_context_draw_init(struct pipe_context *pcontext)
{
   pcontext->draw_vbo = grate_draw_vbo;
}
