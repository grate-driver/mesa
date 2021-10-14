#include <stdio.h>
#include <math.h>

#include "pipe/p_state.h"
#include "util/u_bitcast.h"
#include "util/u_draw.h"
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

static int
grate_init_state(struct grate_context *context)
{
   struct grate_stream *stream = &context->gr3d->stream;

   grate_stream_push_setclass(stream, HOST1X_CLASS_GR3D);

   /* Tegra114 specific stuff */
   grate_stream_push(stream, host1x_opcode_imm(0xe44, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0x807, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc00, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc01, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc02, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc03, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc30, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc31, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc32, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc33, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc40, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc41, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc42, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc43, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc50, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc51, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc52, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xc53, 0x0000));

   grate_stream_push(stream, host1x_opcode_incr(0xe70, 0x0010));
   for (int i = 0; i < 16; i++)
      grate_stream_push(stream, 0x00000000);

   grate_stream_push(stream, host1x_opcode_imm(0xe80, 0x0f00));
   grate_stream_push(stream, host1x_opcode_imm(0xe84, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xe85, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xe86, 0x0000));
   grate_stream_push(stream, host1x_opcode_imm(0xe87, 0x0000));

   /* Tegra30 specific stuff */
   grate_stream_push(stream, host1x_opcode_imm(0x907, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x908, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x909, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x90a, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x90b, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb00, 0x3));

   /*
    * 0x75x should be written after 0xb00, otherwise non-pow2
    * textures are corrupted. Reason is unknown. Looks like
    * combination of 0x75x register bits affects the texture size.
    *
    * The 0x75x registers contain garbage after machine's power-off,
    * but values are retained on soft reboot.
    */
   grate_stream_push(stream, host1x_opcode_incr(0x750, 16));
   for (int i = 0; i < 16; i++)
      grate_stream_push(stream, 0x00000000);

   /*
    * Tegra114 has additional texture descriptors. The order may
    * be important,hence it's placed in a middle of T30 regs until
    * we'll know that this is unnecessary.
    */
   grate_stream_push(stream, host1x_opcode_incr(0x770, 0x0030));
   for (int i = 0; i < 16 + 2 * 16; i++)
      grate_stream_push(stream, 0x00000000);

   grate_stream_push(stream, host1x_opcode_imm(0x7e0, 0x0001));
   grate_stream_push(stream, host1x_opcode_imm(0x7e1, 0x0000));

   grate_stream_push(stream, host1x_opcode_imm(0xb01, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb04, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb06, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb07, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb08, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb09, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb0a, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb0b, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb0c, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb0d, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb0e, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb0f, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb10, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb11, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb12, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb14, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe40, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe41, 0));

   /* Common stuff */
   grate_stream_push(stream, host1x_opcode_imm(0x00d, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x00e, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x00f, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x010, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x011, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x012, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x013, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x014, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x015, 0));

   grate_stream_push(stream, host1x_opcode_imm(TGR3D_VP_ATTRIB_IN_OUT_SELECT, 0));
   grate_stream_push(stream, host1x_opcode_imm(TGR3D_DRAW_PARAMS, 0));

   grate_stream_push(stream, host1x_opcode_imm(0x124, 0x7));
   grate_stream_push(stream, host1x_opcode_imm(0x125, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x126, 0));

   grate_stream_push(stream, host1x_opcode_incr(0x200, 5));
   grate_stream_push(stream, 0x00000011);
   grate_stream_push(stream, 0x0000ffff);
   grate_stream_push(stream, 0x00ff0000);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);

   grate_stream_push(stream, host1x_opcode_imm(0x209, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x20a, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x20b, 0x3));
   grate_stream_push(stream, host1x_opcode_imm(TGR3D_LINKER_INSTRUCTION(0), 0));
   grate_stream_push(stream, host1x_opcode_imm(TGR3D_LINKER_INSTRUCTION(1), 0));

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_CULL_FACE_LINKER_SETUP, 25));
   grate_stream_push(stream, 0xb8e00000); /* TGR3D_CULL_FACE_LINKER_SETUP */
   grate_stream_push(stream, 0x00000000); /* TGR3D_POLYGON_OFFSET_UNITS */
   grate_stream_push(stream, 0x00000000); /* TGR3D_POLYGON_OFFSET_FACTOR */
   grate_stream_push(stream, 0x00000105); /* TGR3D_POINT_PARAMS */
   grate_stream_push(stream, u_bitcast_f2u(0.5f)); /* TGR3D_POINT_SIZE */
   grate_stream_push(stream, u_bitcast_f2u(1.0f)); /* TGR3D_POIN_COORD_RANGE_MAX_S */
   grate_stream_push(stream, u_bitcast_f2u(1.0f)); /* TGR3D_POIN_COORD_RANGE_MAX_T */
   grate_stream_push(stream, u_bitcast_f2u(0.0f)); /* TGR3D_POIN_COORD_RANGE_MIN_S */
   grate_stream_push(stream, u_bitcast_f2u(0.0f)); /* TGR3D_POIN_COORD_RANGE_MIN_T */
   grate_stream_push(stream, 0x00000000); /* TGR2D_LINE_PARAMS */
   grate_stream_push(stream, u_bitcast_f2u(0.5f)); /* TGR3D_HALF_LINE_WIDTH */
   grate_stream_push(stream, u_bitcast_f2u(1.0f)); /* 0x34e - unknonwn */
   grate_stream_push(stream, 0x00000000); /* 0x34f - unknown */
   grate_stream_push(stream, 0x00000000); /* TGR3D_SCISSOR_HORIZ */
   grate_stream_push(stream, 0x00000000); /* TGR3D_SCISSOR_VERT */
   grate_stream_push(stream, u_bitcast_f2u(0.0f)); /* TGR3D_VIEWPORT_X_BIAS */
   grate_stream_push(stream, u_bitcast_f2u(0.0f)); /* TGR3D_VIEWPORT_Y_BIAS */
   grate_stream_push(stream, u_bitcast_f2u(0.5f - powf(2.0, -21))); /* TGR3D_VIEWPORT_Z_BIAS */
   grate_stream_push(stream, u_bitcast_f2u(0.0f)); /* TGR3D_VIEWPORT_X_SCALE */
   grate_stream_push(stream, u_bitcast_f2u(0.0f)); /* TGR3D_VIEWPORT_Y_SCALE */
   grate_stream_push(stream, u_bitcast_f2u(0.5f - powf(2.0, -21))); /* TGR3D_VIEWPORT_Z_SCALE */
   grate_stream_push(stream, u_bitcast_f2u(1.0f)); /* TGR3D_GUARDBAND_WIDTH */
   grate_stream_push(stream, u_bitcast_f2u(1.0f)); /* TGR3D_GUARDBAND_HEIGHT */
   grate_stream_push(stream, u_bitcast_f2u(1.0f)); /* TGR3D_GUARDBAND_DEPTH */
   grate_stream_push(stream, 0x00000205); /* 0x35b - unknown */

   grate_stream_push(stream, host1x_opcode_imm(0x363, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x364, 0));

   grate_stream_push(stream, host1x_opcode_imm(TGR3D_STENCIL_FRONT1, 0x07ff));
   grate_stream_push(stream, host1x_opcode_imm(TGR3D_STENCIL_BACK1, 0x07ff));

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_STENCIL_PARAMS, 18));
   grate_stream_push(stream, 0x00000040); /* TGR3D_STENCIL_PARAMS */
   grate_stream_push(stream, 0x00000310); /* TGR3D_DEPTH_TEST_PARAMS*/
   grate_stream_push(stream, 0x00000000); /* TGR3D_DEPTH_RANGE_NEAR */
   grate_stream_push(stream, 0x000fffff); /* TGR3D_DEPTH_RANGE_FAR */
   grate_stream_push(stream, 0x00000001); /* 0x406 - unknown */
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x1fff1fff);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000006);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000008);
   grate_stream_push(stream, 0x00000048);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);

   grate_stream_push(stream, host1x_opcode_imm(TGR3D_FP_PSEQ_UPLOAD_INST_BUFFER_FLUSH, 0));

   if (context->tegra114)
      grate_stream_push(stream, host1x_opcode_imm(0x501, (0x2200 << 16) | 0x7));
   else
      grate_stream_push(stream, host1x_opcode_imm(0x501, 0x7));

   grate_stream_push(stream, host1x_opcode_imm(0x502, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x503, 0));

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_FP_PSEQ_ENGINE_INST, 32));
   for (int i = 0; i < 32; i++)
      grate_stream_push(stream, 0);

   grate_stream_push(stream, host1x_opcode_imm(0x540, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x542, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x543, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x544, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x545, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x546, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x60e, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x702, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x740, 0x1));
   grate_stream_push(stream, host1x_opcode_imm(0x741, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x742, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x902, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x903, 0));

   grate_stream_push(stream, host1x_opcode_incr(0xa00, 13));
   grate_stream_push(stream, 0x00000e00); /* TGR3D_FDC_CONTROL */
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x000001ff);
   grate_stream_push(stream, 0x000001ff);
   grate_stream_push(stream, 0x000001ff);
   grate_stream_push(stream, 0x00000030);
   grate_stream_push(stream, 0x00000020);
   grate_stream_push(stream, 0x000001ff);
   grate_stream_push(stream, 0x00000100);
   grate_stream_push(stream, 0x0f0f0f0f);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);
   grate_stream_push(stream, 0x00000000);

   grate_stream_push(stream, host1x_opcode_imm(0xe20, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe21, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe22, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe25, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe26, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe27, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe28, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xe29, 0));

   if (context->tegra114) {
      grate_stream_push(stream, host1x_opcode_imm(0x41a, 0xa00));
      grate_stream_push(stream, host1x_opcode_imm(0x416, 0x140));
   }

   return 0;
}

static void
grate_draw_vbo(struct pipe_context *pcontext,
               const struct pipe_draw_info *info,
               unsigned drawid_offset,
               const struct pipe_draw_indirect_info *indirect,
               const struct pipe_draw_start_count_bias *draws,
               unsigned num_draws)
{
   int err;
   uint32_t value;
   struct grate_context *context = grate_context(pcontext);
   struct grate_stream *stream = &context->gr3d->stream;
   uint16_t out_mask = context->vshader->output_mask;
   unsigned int index_size;

   if (num_draws > 1) {
      util_draw_multi(pcontext, info, drawid_offset, indirect, draws, num_draws);
      return;
   }

   if (!indirect && (!draws[0].count || !info->instance_count))
      return;

   if (info->mode >= PIPE_PRIM_QUADS) {
      // the HW can handle non-trimmed sizes, but pimconvert can't
      if (!u_trim_pipe_prim(info->mode, (unsigned*)&draws[0].count))
         return;

      util_primconvert_save_rasterizer_state(context->primconvert, &context->rast->base);
      util_primconvert_draw_vbo(context->primconvert, info, drawid_offset, indirect, draws, num_draws);
      return;
   }

   err = grate_stream_begin(stream);
   if (err < 0) {
      fprintf(stderr, "grate_stream_begin() failed: %d\n", err);
      return;
   }

   /*
    * The state needs to be re-initialized on each draw since tegra doesn't
    * support context switching in a good way, hardware is optimized for
    * uploading.
    */
   grate_init_state(context);
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
