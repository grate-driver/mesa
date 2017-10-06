#include <stdio.h>
#include <math.h>

#include "util/u_bitcast.h"
#include "util/u_helpers.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_format.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_resource.h"
#include "tegra_state.h"

#include "tgr_3d.xml.h"
#include "host1x01_hardware.h"

static void
tegra_set_sample_mask(struct pipe_context *pcontext,
                      unsigned int sample_mask)
{
   unimplemented();
}

static void
tegra_set_constant_buffer(struct pipe_context *pcontext,
                          unsigned int shader, unsigned int index,
                          const struct pipe_constant_buffer *buffer)
{
   struct tegra_context *context = tegra_context(pcontext);

   assert(index == 0);
   assert(!buffer || buffer->user_buffer);

   util_copy_constant_buffer(&context->constant_buffer[shader], buffer);
}

static void
tegra_set_framebuffer_state(struct pipe_context *pcontext,
                            const struct pipe_framebuffer_state *framebuffer)
{
   struct tegra_context *context = tegra_context(pcontext);
   struct pipe_framebuffer_state *cso = &context->framebuffer.base;
   unsigned int i;
   uint32_t mask = 0;

   if (framebuffer->zsbuf) {
      struct tegra_resource *res = tegra_resource(framebuffer->zsbuf->texture);
      uint32_t rt_params;

      rt_params  = TGR3D_VAL(RT_PARAMS, FORMAT, res->format);
      rt_params |= TGR3D_VAL(RT_PARAMS, PITCH, res->pitch);
      rt_params |= TGR3D_BOOL(RT_PARAMS, TILED, res->tiled);

      context->framebuffer.rt_params[0] = rt_params;
      context->framebuffer.bos[0] = res->bo;
      mask |= 1;
   } else {
      context->framebuffer.rt_params[0] = 0;
      context->framebuffer.bos[0] = NULL;
   }

   pipe_surface_reference(&context->framebuffer.base.zsbuf,
                          framebuffer->zsbuf);

   for (i = 0; i < framebuffer->nr_cbufs; i++) {
      struct pipe_surface *ref = framebuffer->cbufs[i];
      struct tegra_resource *res = tegra_resource(ref->texture);
      uint32_t rt_params;

      rt_params  = TGR3D_VAL(RT_PARAMS, FORMAT, res->format);
      rt_params |= TGR3D_VAL(RT_PARAMS, PITCH, res->pitch);
      rt_params |= TGR3D_BOOL(RT_PARAMS, TILED, res->tiled);

      context->framebuffer.rt_params[1 + i] = rt_params;
      context->framebuffer.bos[1 + i] = res->bo;
      mask |= 1 << (1 + i);

      pipe_surface_reference(&cso->cbufs[i], ref);
   }

   for (; i < cso->nr_cbufs; i++)
      pipe_surface_reference(&cso->cbufs[i], NULL);

   context->framebuffer.num_rts = 1 + i;
   context->framebuffer.mask = mask;

   context->framebuffer.base.width = framebuffer->width;
   context->framebuffer.base.height = framebuffer->height;
   context->framebuffer.base.nr_cbufs = framebuffer->nr_cbufs;

   /* prepare the scissor-registers for the non-scissor case */
   context->no_scissor[0]  = host1x_opcode_incr(TGR3D_SCISSOR_HORIZ, 2);
   context->no_scissor[1]  = TGR3D_VAL(SCISSOR_HORIZ, MIN, 0);
   context->no_scissor[1] |= TGR3D_VAL(SCISSOR_HORIZ, MAX, framebuffer->width);
   context->no_scissor[2]  = TGR3D_VAL(SCISSOR_VERT, MIN, 0);
   context->no_scissor[2] |= TGR3D_VAL(SCISSOR_VERT, MAX, framebuffer->height);
}

static void
tegra_set_polygon_stipple(struct pipe_context *pcontext,
                          const struct pipe_poly_stipple *stipple)
{
   unimplemented();
}

static void
tegra_set_scissor_states(struct pipe_context *pcontext,
                         unsigned start_slot,
                         unsigned num_scissors,
                         const struct pipe_scissor_state * scissors)
{
   assert(num_scissors == 1);
   unimplemented();
}

static void
tegra_set_viewport_states(struct pipe_context *pcontext,
                          unsigned start_slot,
                          unsigned num_viewports,
                          const struct pipe_viewport_state *viewports)
{
   struct tegra_context *context = tegra_context(pcontext);
   static const float zeps = powf(2.0f, -21);

   assert(num_viewports == 1);
   assert(start_slot == 0);

   context->viewport[0] = host1x_opcode_incr(TGR3D_VIEWPORT_X_BIAS, 6);
   context->viewport[1] = u_bitcast_f2u(viewports[0].translate[0] * 16.0f);
   context->viewport[2] = u_bitcast_f2u(viewports[0].translate[1] * 16.0f);
   context->viewport[3] = u_bitcast_f2u(viewports[0].translate[2] - zeps);
   context->viewport[4] = u_bitcast_f2u(viewports[0].scale[0] * 16.0f);
   context->viewport[5] = u_bitcast_f2u(viewports[0].scale[1] * 16.0f);
   context->viewport[6] = u_bitcast_f2u(viewports[0].scale[2] - zeps);

   uint32_t depth_near = (viewports[0].translate[2] - viewports[0].scale[2]) * ((1 << 20) - 1);
   uint32_t depth_far = (viewports[0].translate[2] + viewports[0].scale[2]) * ((1 << 20) - 1);
   context->viewport[7] = host1x_opcode_incr(TGR3D_DEPTH_RANGE_NEAR, 2);
   context->viewport[8] = depth_near;
   context->viewport[9] = depth_far;

   assert(viewports[0].scale[0] >= 0.0f);
   float max_x = fabs(viewports[0].translate[0]);
   float max_y = fabs(viewports[0].translate[1]);
   float scale_x = viewports[0].scale[0];
   float scale_y = fabs(viewports[0].scale[1]);
   context->guardband[0] = host1x_opcode_incr(TGR3D_GUARDBAND_WIDTH, 3);
   context->guardband[1] = u_bitcast_f2u((3967 - max_x) / scale_x);
   context->guardband[2] = u_bitcast_f2u((3967 - max_y) / scale_y);
   context->guardband[3] = u_bitcast_f2u(6.99);
}

static void
tegra_set_vertex_buffers(struct pipe_context *pcontext,
                         unsigned int start, unsigned int count,
                         const struct pipe_vertex_buffer *buffer)
{
   struct tegra_context *context = tegra_context(pcontext);
   struct tegra_vertexbuf_state *vbs = &context->vbs;

   util_set_vertex_buffers_mask(vbs->vb, &vbs->enabled, buffer, start, count);
   vbs->count = util_last_bit(vbs->enabled);
}


static void
tegra_set_sampler_views(struct pipe_context *pcontext,
                        unsigned shader,
                        unsigned start_slot, unsigned num_views,
                        struct pipe_sampler_view **views)
{
   unimplemented();
}

void
tegra_context_state_init(struct pipe_context *pcontext)
{
   pcontext->set_sample_mask = tegra_set_sample_mask;
   pcontext->set_constant_buffer = tegra_set_constant_buffer;
   pcontext->set_framebuffer_state = tegra_set_framebuffer_state;
   pcontext->set_polygon_stipple = tegra_set_polygon_stipple;
   pcontext->set_scissor_states = tegra_set_scissor_states;
   pcontext->set_viewport_states = tegra_set_viewport_states;
   pcontext->set_sampler_views = tegra_set_sampler_views;
   pcontext->set_vertex_buffers = tegra_set_vertex_buffers;
}

static void *
tegra_create_blend_state(struct pipe_context *pcontext,
                         const struct pipe_blend_state *template)
{
   struct pipe_blend_state *so = CALLOC_STRUCT(pipe_blend_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
tegra_bind_blend_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
}

static void
tegra_delete_blend_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
tegra_context_blend_init(struct pipe_context *pcontext)
{
   pcontext->create_blend_state = tegra_create_blend_state;
   pcontext->bind_blend_state = tegra_bind_blend_state;
   pcontext->delete_blend_state = tegra_delete_blend_state;
}

static void *
tegra_create_sampler_state(struct pipe_context *pcontext,
            const struct pipe_sampler_state *template)
{
   struct pipe_sampler_state *so = CALLOC_STRUCT(pipe_sampler_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
tegra_bind_sampler_states(struct pipe_context *pcontext,
                          unsigned shader, unsigned start_slot,
                          unsigned num_samplers, void **samplers)
{
   unimplemented();
}

static void
tegra_delete_sampler_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
tegra_context_sampler_init(struct pipe_context *pcontext)
{
   pcontext->create_sampler_state = tegra_create_sampler_state;
   pcontext->bind_sampler_states = tegra_bind_sampler_states;
   pcontext->delete_sampler_state = tegra_delete_sampler_state;
}

static int
tegra_cull_face(int cull_face, bool front_ccw)
{
   switch (cull_face) {
   case PIPE_FACE_NONE:
      return TGR3D_CULL_FACE_NONE;

   case PIPE_FACE_FRONT:
      return front_ccw ? TGR3D_CULL_FACE_CCW : TGR3D_CULL_FACE_CW;

   case PIPE_FACE_BACK:
      return front_ccw ? TGR3D_CULL_FACE_CW : TGR3D_CULL_FACE_CCW;

   case PIPE_FACE_FRONT_AND_BACK:
      return TGR3D_CULL_FACE_BOTH;

   default:
      unreachable("unknown cull_face");
   }
}

static void *
tegra_create_rasterizer_state(struct pipe_context *pcontext,
                              const struct pipe_rasterizer_state *template)
{
   struct tegra_rasterizer_state *so = CALLOC_STRUCT(tegra_rasterizer_state);
   if (!so)
      return NULL;

   so->base = *template;

   so->draw_params = TGR3D_VAL(DRAW_PARAMS, PROVOKING_VERTEX, !template->flatshade_first);

   so->cull_face = TGR3D_BOOL(CULL_FACE_LINKER_SETUP, FRONT_CW, !template->front_ccw);
   so->cull_face |= TGR3D_VAL(CULL_FACE_LINKER_SETUP, CULL_FACE, tegra_cull_face(template->cull_face, template->front_ccw));

   return so;
}

static void
tegra_bind_rasterizer_state(struct pipe_context *pcontext, void *so)
{
   tegra_context(pcontext)->rast = so;
}

static void
tegra_delete_rasterizer_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
tegra_context_rasterizer_init(struct pipe_context *pcontext)
{
   pcontext->create_rasterizer_state = tegra_create_rasterizer_state;
   pcontext->bind_rasterizer_state = tegra_bind_rasterizer_state;
   pcontext->delete_rasterizer_state = tegra_delete_rasterizer_state;
}

static int
tegra_compare_func(enum pipe_compare_func func)
{
   switch (func) {
   case PIPE_FUNC_NEVER: return TGR3D_COMPARE_FUNC_NEVER;
   case PIPE_FUNC_LESS: return TGR3D_COMPARE_FUNC_LESS;
   case PIPE_FUNC_EQUAL: return TGR3D_COMPARE_FUNC_EQUAL;
   case PIPE_FUNC_LEQUAL: return TGR3D_COMPARE_FUNC_LEQUAL;
   case PIPE_FUNC_GREATER: return TGR3D_COMPARE_FUNC_GREATER;
   case PIPE_FUNC_NOTEQUAL: return TGR3D_COMPARE_FUNC_NOTEQUAL;
   case PIPE_FUNC_ALWAYS: return TGR3D_COMPARE_FUNC_ALWAYS;
   default: unreachable("unknown pipe_compare_func");
   }
}

static void *
tegra_create_zsa_state(struct pipe_context *pcontext,
                       const struct pipe_depth_stencil_alpha_state *template)
{
   struct tegra_zsa_state *so = CALLOC_STRUCT(tegra_zsa_state);
   if (!so)
      return NULL;

   so->base = *template;

   uint32_t depth_test = 0;
   depth_test |= TGR3D_VAL(DEPTH_TEST_PARAMS, FUNC,
                           tegra_compare_func(template->depth.func));
   depth_test |= TGR3D_BOOL(DEPTH_TEST_PARAMS, DEPTH_TEST,
                            template->depth.enabled);
   depth_test |= TGR3D_BOOL(DEPTH_TEST_PARAMS, DEPTH_WRITE,
                            template->depth.writemask);
   depth_test |= 0x200;

   so->commands[0] = host1x_opcode_incr(TGR3D_DEPTH_TEST_PARAMS, 1);
   so->commands[1] = depth_test;

   return so;
}

static void
tegra_bind_zsa_state(struct pipe_context *pcontext, void *so)
{
   tegra_context(pcontext)->zsa = so;
}

static void
tegra_delete_zsa_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
tegra_context_zsa_init(struct pipe_context *pcontext)
{
   pcontext->create_depth_stencil_alpha_state = tegra_create_zsa_state;
   pcontext->bind_depth_stencil_alpha_state = tegra_bind_zsa_state;
   pcontext->delete_depth_stencil_alpha_state = tegra_delete_zsa_state;
}

/*
 * Note: this does not include the stride, which needs to be mixed in later
 **/
static uint32_t
tegra_attrib_mode(const struct pipe_vertex_element *e)
{
   const struct util_format_description *desc = util_format_description(e->src_format);
   const int c = util_format_get_first_non_void_channel(e->src_format);
   uint32_t type, format;

   assert(!desc->is_mixed);
   assert(c >= 0);

   switch (desc->channel[c].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
   case UTIL_FORMAT_TYPE_SIGNED:
      switch (desc->channel[c].size) {
      case 8:
         type = TGR3D_ATTRIB_TYPE_UBYTE;
         break;

      case 16:
         type = TGR3D_ATTRIB_TYPE_USHORT;
         break;

      case 32:
         type = TGR3D_ATTRIB_TYPE_UINT;
         break;

      default:
         unreachable("invalid channel-size");
      }

      if (desc->channel[c].type == UTIL_FORMAT_TYPE_SIGNED)
         type += 2;

      if (desc->channel[c].normalized)
         type += 1;

      break;

   case UTIL_FORMAT_TYPE_FIXED:
      assert(desc->channel[c].size == 32);
      type = TGR3D_ATTRIB_TYPE_FIXED16;
      break;

   case UTIL_FORMAT_TYPE_FLOAT:
      assert(desc->channel[c].size == 32); /* TODO: float16 ? */
      type = TGR3D_ATTRIB_TYPE_FLOAT32;
      break;

   default:
      unreachable("invalid channel-type");
   }

   format  = TGR3D_VAL(ATTRIB_MODE, TYPE, type);
   format |= TGR3D_VAL(ATTRIB_MODE, SIZE, desc->nr_channels);
   return format;
}

static void *
tegra_create_vertex_state(struct pipe_context *pcontext, unsigned int count,
                          const struct pipe_vertex_element *elements)
{
   unsigned int i;
   struct tegra_vertex_state *vtx = CALLOC_STRUCT(tegra_vertex_state);
   if (!vtx)
      return NULL;

   for (i = 0; i < count; ++i) {
      const struct pipe_vertex_element *src = elements + i;
      struct tegra_vertex_element *dst = vtx->elements + i;
      dst->attrib = tegra_attrib_mode(src);
      dst->buffer_index = src->vertex_buffer_index;
      dst->offset = src->src_offset;
   }

   vtx->num_elements = count;

   return vtx;
}

static void
tegra_bind_vertex_state(struct pipe_context *pcontext, void *so)
{
   tegra_context(pcontext)->vs = so;
}

static void
tegra_delete_vertex_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

static void
emit_attribs(struct tegra_context *context)
{
   unsigned int i;
   struct tegra_stream *stream = &context->gr3d->stream;

   assert(context->vs);

   for (i = 0; i < context->vs->num_elements; ++i) {
      const struct pipe_vertex_buffer *vb;
      const struct tegra_vertex_element *e = context->vs->elements + i;
      const struct tegra_resource *r;

      assert(e->buffer_index < context->vbs.count);
      vb = context->vbs.vb + e->buffer_index;
      assert(!vb->is_user_buffer);
      r = tegra_resource(vb->buffer.resource);

      uint32_t attrib = e->attrib;
      assert(vb->stride < 1 << 24);
      attrib |= TGR3D_VAL(ATTRIB_MODE, STRIDE, vb->stride);

      tegra_stream_push(stream, host1x_opcode_incr(TGR3D_ATTRIB_PTR(i), 2));
      tegra_stream_push_reloc(stream, r->bo, vb->buffer_offset + e->offset);
      tegra_stream_push(stream, attrib);
   }
}

static void
emit_render_targets(struct tegra_context *context)
{
   unsigned int i;
   struct tegra_stream *stream = &context->gr3d->stream;
   const struct tegra_framebuffer_state *fb = &context->framebuffer;

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_RT_PARAMS(0), fb->num_rts));
   for (i = 0; i < fb->num_rts; ++i) {
      uint32_t rt_params = fb->rt_params[i];
      /* TODO: setup dither */
      /* rt_params |= TGR3D_BOOL(RT_PARAMS, DITHER_ENABLE, enable_dither); */
      tegra_stream_push(stream, rt_params);
   }

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_RT_PTR(0), fb->num_rts));
   for (i = 0; i < fb->num_rts; ++i)
      tegra_stream_push_reloc(stream, fb->bos[i], 0);

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_RT_ENABLE, 1));
   tegra_stream_push(stream, fb->mask);
}

static void
emit_scissor(struct tegra_context *context)
{
   struct tegra_stream *stream = &context->gr3d->stream;
   tegra_stream_push_words(stream, context->no_scissor, 3, 0);
}

static void
emit_viewport(struct tegra_context *context)
{
   struct tegra_stream *stream = &context->gr3d->stream;
   tegra_stream_push_words(stream, context->viewport, 10, 0);
}

static void
emit_guardband(struct tegra_context *context)
{
   struct tegra_stream *stream = &context->gr3d->stream;
   tegra_stream_push_words(stream, context->guardband, 4, 0);
}

static void
emit_zsa_state(struct tegra_context *context)
{
   struct tegra_stream *stream = &context->gr3d->stream;
   tegra_stream_push_words(stream, context->zsa->commands, 2, 0);
}

static void
emit_vs_uniforms(struct tegra_context *context)
{
   struct tegra_stream *stream = &context->gr3d->stream;
   struct pipe_constant_buffer *constbuf = &context->constant_buffer[PIPE_SHADER_VERTEX];
   int len;

   if (constbuf->user_buffer != NULL) {
      assert(constbuf->buffer_size % sizeof(uint32_t) == 0);

      len = constbuf->buffer_size / 4;
      assert(len < 256 * 4);

      tegra_stream_push(stream, host1x_opcode_imm(TGR3D_VP_UPLOAD_CONST_ID, 0));
      tegra_stream_push(stream, host1x_opcode_nonincr(TGR3D_VP_UPLOAD_CONST, len));
      tegra_stream_push_words(stream, constbuf->user_buffer, len, 0);
   }
}

void
tegra_emit_state(struct tegra_context *context)
{
   emit_render_targets(context);
   emit_viewport(context);
   emit_guardband(context);
   emit_scissor(context);
   emit_zsa_state(context);
   emit_attribs(context);
   emit_vs_uniforms(context);
}

void
tegra_context_vbo_init(struct pipe_context *pcontext)
{
   pcontext->create_vertex_elements_state = tegra_create_vertex_state;
   pcontext->bind_vertex_elements_state = tegra_bind_vertex_state;
   pcontext->delete_vertex_elements_state = tegra_delete_vertex_state;
}
