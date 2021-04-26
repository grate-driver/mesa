#include <stdio.h>

#include "pipe/p_state.h"
#include "util/u_helpers.h"
#include "util/u_prim.h"
#include "indices/u_primconvert.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_draw.h"
#include "grate_program.h"
#include "grate_resource.h"
#include "grate_state.h"

#include "tgr_3d.xml.h"
#include "host1x01_hardware.h"

static int
grate_primitive_type(enum pipe_prim_type mode)
{
   switch (mode) {
   case PIPE_PRIM_POINTS:
      return TGR3D_PRIMITIVE_TYPE_POINTS;

   case PIPE_PRIM_LINES:
      return TGR3D_PRIMITIVE_TYPE_LINES;

   case PIPE_PRIM_LINE_LOOP:
      return TGR3D_PRIMITIVE_TYPE_LINE_LOOP;

   case PIPE_PRIM_LINE_STRIP:
      return TGR3D_PRIMITIVE_TYPE_LINE_STRIP;

   case PIPE_PRIM_TRIANGLES:
      return TGR3D_PRIMITIVE_TYPE_TRIANGLES;

   case PIPE_PRIM_TRIANGLE_STRIP:
      return TGR3D_PRIMITIVE_TYPE_TRIANGLE_STRIP;

   case PIPE_PRIM_TRIANGLE_FAN:
      return TGR3D_PRIMITIVE_TYPE_TRIANGLE_FAN;

   default:
      unreachable("unexpected enum pipe_prim_type");
   }
}

static void
grate_draw_vbo(struct pipe_context *pcontext,
               const struct pipe_draw_info *info,
               const struct pipe_draw_indirect_info *indirect,
               const struct pipe_draw_start_count *draws,
               unsigned num_draws)
{
   int err;
   uint32_t value;
   struct grate_context *context = grate_context(pcontext);
   struct grate_stream *stream = &context->gr3d->stream;
   uint16_t out_mask = context->vshader->output_mask;
   unsigned int index_size;

   if (num_draws > 1) {
      struct pipe_draw_info tmp_info = *info;

      for (unsigned i = 0; i < num_draws; i++) {
         grate_draw_vbo(pcontext, &tmp_info, indirect, &draws[i], 1);
         if (tmp_info.increment_draw_id)
            tmp_info.drawid++;
      }
      return;
   }

   if (!indirect && (!draws[0].count || !info->instance_count))
      return;

   if (info->mode >= PIPE_PRIM_QUADS) {
      // the HW can handle non-trimmed sizes, but pimconvert can't
      if (!u_trim_pipe_prim(info->mode, (unsigned*)&draws[0].count))
         return;

      util_primconvert_save_rasterizer_state(context->primconvert, &context->rast->base);
      util_primconvert_draw_vbo(context->primconvert, info, &draws[0]);
      return;
   }

   err = grate_stream_begin(stream);
   if (err < 0) {
      fprintf(stderr, "grate_stream_begin() failed: %d\n", err);
      return;
   }

   grate_stream_push_setclass(stream, HOST1X_CLASS_GR3D);

   grate_emit_state(context);

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_VP_ATTRIB_IN_OUT_SELECT, 1));
   grate_stream_push(stream, ((uint32_t)context->vs->mask << 16) | out_mask);

   struct pipe_resource *index_buffer = NULL;
   unsigned offset = 0;
   if (info->index_size > 0) {
      unsigned index_offset = 0;
      if (info->has_user_indices) {
         if (!util_upload_index_buffer(pcontext, info, draws, &index_buffer, &index_offset, 64)) {
            fprintf(stderr, "util_upload_index_buffer() failed\n");
            return;
         }
      } else
         index_buffer = info->index.resource;

      index_offset += draws->start * info->index_size;
      grate_stream_push(stream, host1x_opcode_incr(TGR3D_INDEX_PTR, 1));
      grate_stream_push_reloc(stream, grate_resource(index_buffer)->bo, index_offset);
   } else
      offset = draws->start;

   switch (info->index_size) {
   case 1:
      index_size = 1;
      break;

   case 2:
      index_size = 2;
      break;

   case 4:
      index_size = 3;
      break;

   default:
      fprintf(stderr, "grate_draw_vbo() invalid index size: %u\n",
              info->index_size);
      return;
   }

   /* draw params */
   value  = TGR3D_VAL(DRAW_PARAMS, INDEX_MODE, index_size);
   value |= context->rast->draw_params;
   value |= TGR3D_VAL(DRAW_PARAMS, PRIMITIVE_TYPE, grate_primitive_type(info->mode));
   value |= TGR3D_VAL(DRAW_PARAMS, FIRST, draws->start);
   value |= 0xC0000000; /* flush input caches? */

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PARAMS, 1));
   grate_stream_push(stream, value);

   assert(draws->count > 0 && draws->count < (1 << 11));
   value  = TGR3D_VAL(DRAW_PRIMITIVES, INDEX_COUNT, draws->count - 1);
   value |= TGR3D_VAL(DRAW_PRIMITIVES, OFFSET, offset);
   grate_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PRIMITIVES, 1));
   grate_stream_push(stream, value);

   grate_stream_end(stream);

   grate_stream_flush(stream);
}

void
grate_context_draw_init(struct pipe_context *pcontext)
{
   pcontext->draw_vbo = grate_draw_vbo;
}
