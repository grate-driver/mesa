#include <stdio.h>
#include <math.h>

#include "pipe/p_state.h"
#include "util/u_bitcast.h"
#include "util/u_draw.h"
#include "util/u_helpers.h"
#include "util/u_inlines.h"
#include "util/u_prim.h"
#include "indices/u_primconvert.h"

#include "grate_common.h"
#include "grate_diagnostics.h"
#include "grate_context.h"
#include "grate_draw.h"
#include "grate_draw_limits.h"
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
   /* GR2D clears and other contexts can change render-target memory.
    * Discard stale fragment data before framebuffer fetch (OpenTegra init). */
   grate_stream_push(stream, 0x00000e00 | TGR3D_FDC_CONTROL_INVALIDATE);
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
   struct grate_stream *stream;
   uint16_t out_mask;
   unsigned int index_size;

   if (!num_draws || (!indirect && !info->instance_count)) return;
   if (indirect || info->instance_count != 1 || info->start_instance ||
       info->primitive_restart) {
      fprintf(stderr,"GRATE_DIAG STOP: unsupported indirect/instanced/restart draw\n");
      abort();
   }
   if (num_draws > 1) {
      util_draw_multi(pcontext, info, drawid_offset, indirect, draws, num_draws);
      return;
   }

   if (!draws[0].count) return;
   struct pipe_draw_start_count_bias draw = draws[0];
   if ((uint64_t)draw.start + draw.count > UINT_MAX) {
      fprintf(stderr,"GRATE_DIAG STOP: draw range overflows start/count\n");
      abort();
   }
   if (info->index_size && ((info->index_size != 1 && info->index_size != 2 &&
                            info->index_size != 4) ||
                           (info->has_user_indices && !info->index.user))) {
      fprintf(stderr,"GRATE_DIAG STOP: invalid index type/pointer\n");
      abort();
   }
   if (info->index_size && ((uint64_t)draw.start+draw.count)*info->index_size>UINT_MAX) {
      fprintf(stderr,"GRATE_DIAG STOP: index byte range overflows address space\n");
      abort();
   }
   if (info->index_size && draw.index_bias != 0) {
      fprintf(stderr,"GRATE_DIAG STOP: nonzero indexed base vertex is unverified\n");
      abort();
   }
   if (info->index_size && !info->has_user_indices) {
      if (!info->index.resource) abort();
      struct grate_resource *indices = grate_resource(info->index.resource);
      uint32_t size = 0;
      if (drm_tegra_bo_get_size(indices->bo,&size) ||
          !grate_index_range_valid(draw.start,draw.count,info->index_size,
                                   indices->b.width0,size)) {
         fprintf(stderr,"GRATE_DIAG STOP: index range exceeds resource/BO\n");
         abort();
      }
   }
   if (!info->index_size && (uint64_t)draw.start + draw.count >
       (uint64_t)TGR3D_DRAW_PRIMITIVES_OFFSET__MASK + 1) {
      fprintf(stderr,"GRATE_DIAG STOP: nonindexed start exceeds command field\n");
      abort();
   }
   if (info->mode >= PIPE_PRIM_QUADS) {
      // the HW can handle non-trimmed sizes, but pimconvert can't
      if (!u_trim_pipe_prim(info->mode, &draw.count))
         return;

      util_primconvert_save_rasterizer_state(context->primconvert, &context->rast->base);
      util_primconvert_draw_vbo(context->primconvert, info, drawid_offset, indirect, &draw, 1);
      return;
   }

   if (!u_trim_pipe_prim(info->mode,&draw.count)) return;
   if (draw.count > GRATE_DRAW_LIMIT) {
      unsigned remaining=draw.count,start=draw.start;
      while (remaining) {
         unsigned count,step;
         if (!grate_next_draw_chunk(info->mode,remaining,&count,&step)) {
            fprintf(stderr,"GRATE_DIAG STOP: large primitive needs index conversion\n");
            abort();
         }
         struct pipe_draw_start_count_bias part=draw;
         part.start=start; part.count=count;
         grate_draw_vbo(pcontext,info,drawid_offset,NULL,&part,1);
         remaining-=step; start+=step;
      }
      return;
   }
   draws=&draw;
   stream=&context->gr3d->stream;
   out_mask=context->vshader->output_mask;

   if (grate_diagnostics_enabled()) fprintf(stderr,
      "GRATE_DIAG DRAW mode=%s(%u) start=%u count=%u index_size=%u user_indices=%u instances=%u indirect=%u index_bias=%d\n",
      u_prim_name(info->mode),info->mode,draws->start,draws->count,
      info->index_size,info->has_user_indices,info->instance_count,indirect!=NULL,draws->index_bias);

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
   bool uploaded_indices = false;
   unsigned offset = 0;
   unsigned index_bytes = info->index_size;
   if (info->index_size > 0) {
      unsigned index_offset = 0;
      struct pipe_draw_info upload_info=*info;
      struct pipe_draw_start_count_bias upload_draw=*draws;
      uint8_t copied[GRATE_DRAW_LIMIT*4];
      /* Copy before allocating/unmapping: the source may alias the uploader. */
      size_t source_offset = (size_t)draws->start * info->index_size;
      size_t source_bytes = (size_t)draws->count * info->index_size;
      if (info->has_user_indices) {
         memcpy(copied, (const uint8_t *)info->index.user + source_offset,
                source_bytes);
      } else {
         struct grate_resource *source=grate_resource(info->index.resource);
         void *map=NULL;
         if (drm_tegra_bo_map(source->bo,&map)) abort();
         memcpy(copied,(uint8_t*)map+source_offset,source_bytes);
         if (drm_tegra_bo_unmap(source->bo)) abort();
      }
      if (index_bytes == 4) {
         if (!grate_index32_to_16(copied, draws->count)) {
            fprintf(stderr,"GRATE_DIAG STOP: 32-bit index exceeds verified UINT16 range\n");
            abort();
         }
         index_bytes = 2;
      }
      upload_info.has_user_indices=true;
      upload_info.index_size=index_bytes;
      upload_info.index.user=copied;
      upload_draw.start=0;
      if (!util_upload_index_buffer(pcontext,&upload_info,&upload_draw,
                                     &index_buffer,&index_offset,64)) {
         fprintf(stderr,"GRATE_DIAG STOP: index upload failed\n");
         abort();
      }
      uploaded_indices=true;
      uint64_t pointer=(uint64_t)index_offset+(uint64_t)upload_draw.start*index_bytes;
      uint64_t end=pointer+(uint64_t)draws->count*index_bytes;
      uint32_t size=0;
      if (pointer>UINT_MAX || (pointer&63u) || end>index_buffer->width0 ||
          drm_tegra_bo_get_size(grate_resource(index_buffer)->bo,&size) || end>size) {
         fprintf(stderr,"GRATE_DIAG STOP: uploaded index range exceeds/alignment BO\n");
         abort();
      }
      index_offset=(unsigned)pointer;
      /* The pointer is the first selected index; the primitive offset is zero. */
      offset=0;
      grate_stream_push(stream, host1x_opcode_incr(TGR3D_INDEX_PTR, 1));
      grate_stream_push_reloc(stream, grate_resource(index_buffer)->bo, index_offset);
   } else
      offset = draws->start;

   switch (index_bytes) {
   case 0:
      index_size = 0;
      break;

   case 1:
      index_size = 1;
      break;

   case 2:
      index_size = 2;
      break;

   default:
      fprintf(stderr, "grate_draw_vbo() invalid index size: %u\n",
              info->index_size);
      return;
   }

   if (context->blend_mode) {
      grate_stream_push(stream,host1x_opcode_imm(0xb00,1));
      grate_stream_push(stream,host1x_opcode_imm(0xe41,1));
      grate_stream_push(stream,host1x_opcode_imm(0xb00,2));
      grate_stream_push(stream,host1x_opcode_imm(0xe41,3));
      grate_stream_push(stream,host1x_opcode_imm(0xb00,3));
   }
   /* draw params */
   value  = TGR3D_VAL(DRAW_PARAMS, INDEX_MODE, index_size);
   value |= context->rast->draw_params;
   value |= TGR3D_VAL(DRAW_PARAMS, PRIMITIVE_TYPE, grate_primitive_type(info->mode));
   value |= TGR3D_VAL(DRAW_PARAMS, FIRST, 0);
   value |= 0xC0000000; /* flush input caches? */

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PARAMS, 1));
   grate_stream_push(stream, value);

   assert(draws->count > 0 && draws->count < (1 << 11));
   value  = TGR3D_VAL(DRAW_PRIMITIVES, INDEX_COUNT, draws->count - 1);
   value |= TGR3D_VAL(DRAW_PRIMITIVES, OFFSET, offset);
   grate_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PRIMITIVES, 1));
   grate_stream_push(stream, value);

   if (grate_stream_end(stream) || grate_stream_flush(stream)) {
      fprintf(stderr,"GRATE_DIAG STOP: draw submission failed\n");
      abort();
   }
   /* Submission waits synchronously: drop only the upload's owned reference. */
   if (uploaded_indices) pipe_resource_reference(&index_buffer,NULL);
}

void
grate_context_draw_init(struct pipe_context *pcontext)
{
   pcontext->draw_vbo = grate_draw_vbo;
}
