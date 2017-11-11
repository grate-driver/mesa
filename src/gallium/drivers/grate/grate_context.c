#include <errno.h>
#include <stdio.h>
#include <math.h>

#include "util/u_bitcast.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_draw.h"
#include "grate_program.h"
#include "grate_resource.h"
#include "grate_screen.h"
#include "grate_state.h"
#include "grate_surface.h"

#include "host1x01_hardware.h"
#include "tgr_3d.xml.h"

static int
init(struct grate_stream *stream)
{
   int err = grate_stream_begin(stream);
   if (err < 0) {
      fprintf(stderr, "grate_stream_begin() failed: %d\n", err);
      return err;
   }

   grate_stream_push_setclass(stream, HOST1X_CLASS_GR3D);

   /* Tegra30 specific stuff */
   grate_stream_push(stream, host1x_opcode_incr(0x750, 16));
   for (int i = 0; i < 16; i++)
      grate_stream_push(stream, 0x00000000);

   grate_stream_push(stream, host1x_opcode_imm(0x907, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x908, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x909, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x90a, 0));
   grate_stream_push(stream, host1x_opcode_imm(0x90b, 0));
   grate_stream_push(stream, host1x_opcode_imm(0xb00, 0x3));
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

   grate_stream_end(stream);
   grate_stream_flush(stream);

   return 0;
}

static int
grate_channel_create(struct grate_context *context,
                     enum drm_tegra_class class,
                     struct grate_channel **channelp)
{
   struct grate_screen *screen = grate_screen(context->base.screen);
   int err;
   struct drm_tegra_channel *drm_channel;
   struct grate_channel *channel;

   err = drm_tegra_channel_open(&drm_channel, screen->drm, class);
   if (err < 0)
      return err;

   channel = CALLOC_STRUCT(grate_channel);
   if (!channel)
      return -ENOMEM;

   channel->context = context;

   err = grate_stream_create(screen->drm, drm_channel, &channel->stream, 32768);
   if (err < 0) {
      FREE(channel);
      drm_tegra_channel_close(drm_channel);
      return err;
   }

   *channelp = channel;

   return 0;
}

static void
grate_channel_delete(struct grate_channel *channel)
{
   grate_stream_destroy(&channel->stream);
   drm_tegra_channel_close(channel->stream.channel);
   FREE(channel);
}

static void
grate_context_destroy(struct pipe_context *pcontext)
{
   struct grate_context *context = grate_context(pcontext);

   slab_destroy_child(&context->transfer_pool);

   grate_channel_delete(context->gr3d);
   grate_channel_delete(context->gr2d);
   FREE(context);
}

static void
grate_context_flush(struct pipe_context *pcontext,
                    struct pipe_fence_handle **pfence,
                    enum pipe_flush_flags flags)
{
   unimplemented();
}

struct pipe_context *
grate_screen_context_create(struct pipe_screen *pscreen,
                            void *priv, unsigned flags)
{
   struct grate_screen *screen = grate_screen(pscreen);
   int err;

   struct grate_context *context = CALLOC_STRUCT(grate_context);
   if (!context)
      return NULL;

   context->base.screen = pscreen;
   context->base.priv = priv;

   err = grate_channel_create(context, DRM_TEGRA_GR2D, &context->gr2d);
   if (err < 0) {
      fprintf(stderr, "grate_channel_create() failed: %d\n", err);
      return NULL;
   }

   err = grate_channel_create(context, DRM_TEGRA_GR3D, &context->gr3d);
   if (err < 0) {
      fprintf(stderr, "grate_channel_create() failed: %d\n", err);
      return NULL;
   }

   init(&context->gr3d->stream);

   slab_create_child(&context->transfer_pool, &screen->transfer_pool);

   context->base.destroy = grate_context_destroy;
   context->base.flush = grate_context_flush;
   context->base.stream_uploader = u_upload_create_default(&context->base);
   context->base.const_uploader = context->base.stream_uploader;

   grate_context_resource_init(&context->base);
   grate_context_surface_init(&context->base);
   grate_context_state_init(&context->base);
   grate_context_blend_init(&context->base);
   grate_context_sampler_init(&context->base);
   grate_context_rasterizer_init(&context->base);
   grate_context_zsa_init(&context->base);
   grate_context_program_init(&context->base);
   grate_context_vbo_init(&context->base);
   grate_context_draw_init(&context->base);

   return &context->base;
}
