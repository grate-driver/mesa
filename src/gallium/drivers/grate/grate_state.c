#include <stdio.h>
#include <math.h>

#include "util/u_bitcast.h"
#include "util/u_helpers.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_format.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_resource.h"
#include "grate_state.h"

#include "tgr_3d.xml.h"
#include "host1x01_hardware.h"

static void
grate_set_sample_mask(struct pipe_context *pcontext,
                      unsigned int sample_mask)
{
   unimplemented();
}

static void
grate_set_constant_buffer(struct pipe_context *pcontext,
                          unsigned int shader, unsigned int index,
                          const struct pipe_constant_buffer *buffer)
{
   struct grate_context *context = grate_context(pcontext);

   assert(index == 0);
   assert(!buffer || buffer->user_buffer);

   util_copy_constant_buffer(&context->constant_buffer[shader], buffer);
}

static void
grate_set_framebuffer_state(struct pipe_context *pcontext,
                            const struct pipe_framebuffer_state *framebuffer)
{
   struct grate_context *context = grate_context(pcontext);
   struct pipe_framebuffer_state *cso = &context->framebuffer.base;
   unsigned int i;
   uint32_t mask = 0;

   if (framebuffer->zsbuf) {
      struct grate_resource *res = grate_resource(framebuffer->zsbuf->texture);
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
      struct grate_resource *res = grate_resource(ref->texture);
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
grate_set_polygon_stipple(struct pipe_context *pcontext,
                          const struct pipe_poly_stipple *stipple)
{
   unimplemented();
}

static void
grate_set_scissor_states(struct pipe_context *pcontext,
                         unsigned start_slot,
                         unsigned num_scissors,
                         const struct pipe_scissor_state * scissors)
{
   assert(num_scissors == 1);
   unimplemented();
}

static void
grate_set_viewport_states(struct pipe_context *pcontext,
                          unsigned start_slot,
                          unsigned num_viewports,
                          const struct pipe_viewport_state *viewports)
{
   struct grate_context *context = grate_context(pcontext);
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
grate_set_vertex_buffers(struct pipe_context *pcontext,
                         unsigned int start, unsigned int count,
                         const struct pipe_vertex_buffer *buffer)
{
   struct grate_context *context = grate_context(pcontext);
   struct grate_vertexbuf_state *vbs = &context->vbs;

   util_set_vertex_buffers_mask(vbs->vb, &vbs->enabled, buffer, start, count);
   vbs->count = util_last_bit(vbs->enabled);
}


static void
grate_set_sampler_views(struct pipe_context *pcontext,
                        unsigned shader,
                        unsigned start_slot, unsigned num_views,
                        struct pipe_sampler_view **views)
{
   unimplemented();
}

void
grate_context_state_init(struct pipe_context *pcontext)
{
   pcontext->set_sample_mask = grate_set_sample_mask;
   pcontext->set_constant_buffer = grate_set_constant_buffer;
   pcontext->set_framebuffer_state = grate_set_framebuffer_state;
   pcontext->set_polygon_stipple = grate_set_polygon_stipple;
   pcontext->set_scissor_states = grate_set_scissor_states;
   pcontext->set_viewport_states = grate_set_viewport_states;
   pcontext->set_sampler_views = grate_set_sampler_views;
   pcontext->set_vertex_buffers = grate_set_vertex_buffers;
}

static void *
grate_create_blend_state(struct pipe_context *pcontext,
                         const struct pipe_blend_state *template)
{
   struct pipe_blend_state *so = CALLOC_STRUCT(pipe_blend_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
grate_bind_blend_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
}

static void
grate_delete_blend_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
grate_context_blend_init(struct pipe_context *pcontext)
{
   pcontext->create_blend_state = grate_create_blend_state;
   pcontext->bind_blend_state = grate_bind_blend_state;
   pcontext->delete_blend_state = grate_delete_blend_state;
}

static void *
grate_create_sampler_state(struct pipe_context *pcontext,
            const struct pipe_sampler_state *template)
{
   struct pipe_sampler_state *so = CALLOC_STRUCT(pipe_sampler_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
grate_bind_sampler_states(struct pipe_context *pcontext,
                          unsigned shader, unsigned start_slot,
                          unsigned num_samplers, void **samplers)
{
   unimplemented();
}

static void
grate_delete_sampler_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
grate_context_sampler_init(struct pipe_context *pcontext)
{
   pcontext->create_sampler_state = grate_create_sampler_state;
   pcontext->bind_sampler_states = grate_bind_sampler_states;
   pcontext->delete_sampler_state = grate_delete_sampler_state;
}

static void *
grate_create_rasterizer_state(struct pipe_context *pcontext,
                              const struct pipe_rasterizer_state *template)
{
   struct pipe_rasterizer_state *so = CALLOC_STRUCT(pipe_rasterizer_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
grate_bind_rasterizer_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
}

static void
grate_delete_rasterizer_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
grate_context_rasterizer_init(struct pipe_context *pcontext)
{
   pcontext->create_rasterizer_state = grate_create_rasterizer_state;
   pcontext->bind_rasterizer_state = grate_bind_rasterizer_state;
   pcontext->delete_rasterizer_state = grate_delete_rasterizer_state;
}

static void *
grate_create_zsa_state(struct pipe_context *pcontext,
                       const struct pipe_depth_stencil_alpha_state *template)
{
   struct pipe_depth_stencil_alpha_state *so = CALLOC_STRUCT(pipe_depth_stencil_alpha_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
grate_bind_zsa_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
}

static void
grate_delete_zsa_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
grate_context_zsa_init(struct pipe_context *pcontext)
{
   pcontext->create_depth_stencil_alpha_state = grate_create_zsa_state;
   pcontext->bind_depth_stencil_alpha_state = grate_bind_zsa_state;
   pcontext->delete_depth_stencil_alpha_state = grate_delete_zsa_state;
}

/*
 * Note: this does not include the stride, which needs to be mixed in later
 **/
static uint32_t
attrib_mode(const struct pipe_vertex_element *e)
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
grate_create_vertex_state(struct pipe_context *pcontext, unsigned int count,
                          const struct pipe_vertex_element *elements)
{
   unsigned int i;
   struct grate_vertex_state *vtx = CALLOC_STRUCT(grate_vertex_state);
   if (!vtx)
      return NULL;

   for (i = 0; i < count; ++i) {
      const struct pipe_vertex_element *src = elements + i;
      struct grate_vertex_element *dst = vtx->elements + i;
      dst->attrib = attrib_mode(src);
      dst->buffer_index = src->vertex_buffer_index;
      dst->offset = src->src_offset;
   }

   vtx->num_elements = count;

   return vtx;
}

static void
grate_bind_vertex_state(struct pipe_context *pcontext, void *so)
{
   grate_context(pcontext)->vs = so;
}

static void
grate_delete_vertex_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

static void
emit_attribs(struct grate_context *context)
{
   unsigned int i;
   struct grate_stream *stream = &context->gr3d->stream;

   assert(context->vs);

   for (i = 0; i < context->vs->num_elements; ++i) {
      const struct pipe_vertex_buffer *vb;
      const struct grate_vertex_element *e = context->vs->elements + i;
      const struct grate_resource *r;

      assert(e->buffer_index < context->vbs.count);
      vb = context->vbs.vb + e->buffer_index;
      assert(!vb->is_user_buffer);
      r = grate_resource(vb->buffer.resource);

      uint32_t attrib = e->attrib;
      assert(vb->stride < 1 << 24);
      attrib |= TGR3D_VAL(ATTRIB_MODE, STRIDE, vb->stride);

      grate_stream_push(stream, host1x_opcode_incr(TGR3D_ATTRIB_PTR(i), 2));
      grate_stream_push_reloc(stream, r->bo, vb->buffer_offset + e->offset);
      grate_stream_push(stream, attrib);
   }
}

static void
emit_render_targets(struct grate_context *context)
{
   unsigned int i;
   struct grate_stream *stream = &context->gr3d->stream;
   const struct grate_framebuffer_state *fb = &context->framebuffer;

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_RT_PARAMS(0), fb->num_rts));
   for (i = 0; i < fb->num_rts; ++i) {
      uint32_t rt_params = fb->rt_params[i];
      /* TODO: setup dither */
      /* rt_params |= TGR3D_BOOL(RT_PARAMS, DITHER_ENABLE, enable_dither); */
      grate_stream_push(stream, rt_params);
   }

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_RT_PTR(0), fb->num_rts));
   for (i = 0; i < fb->num_rts; ++i)
      grate_stream_push_reloc(stream, fb->bos[i], 0);

   grate_stream_push(stream, host1x_opcode_incr(TGR3D_RT_ENABLE, 1));
   grate_stream_push(stream, fb->mask);
}

static void
emit_scissor(struct grate_context *context)
{
   struct grate_stream *stream = &context->gr3d->stream;
   grate_stream_push_words(stream, context->no_scissor, 3, 0);
}

static void
emit_viewport(struct grate_context *context)
{
   struct grate_stream *stream = &context->gr3d->stream;
   grate_stream_push_words(stream, context->viewport, 7, 0);
}

static void
emit_guardband(struct grate_context *context)
{
   struct grate_stream *stream = &context->gr3d->stream;
   grate_stream_push_words(stream, context->guardband, 4, 0);
}

static void
emit_vs_uniforms(struct grate_context *context)
{
   struct grate_stream *stream = &context->gr3d->stream;
   struct pipe_constant_buffer *constbuf = &context->constant_buffer[PIPE_SHADER_VERTEX];
   int len;

   if (constbuf->user_buffer != NULL) {
      assert(constbuf->buffer_size % sizeof(uint32_t) == 0);

      len = constbuf->buffer_size / 4;
      assert(len < 256 * 4);

      grate_stream_push(stream, host1x_opcode_imm(TGR3D_VP_UPLOAD_CONST_ID, 0));
      grate_stream_push(stream, host1x_opcode_nonincr(TGR3D_VP_UPLOAD_CONST, len));
      grate_stream_push_words(stream, constbuf->user_buffer, len, 0);
   }
}

void
grate_emit_state(struct grate_context *context)
{
   emit_render_targets(context);
   emit_viewport(context);
   emit_guardband(context);
   emit_scissor(context);
   emit_attribs(context);
   emit_vs_uniforms(context);
}

void
grate_context_vbo_init(struct pipe_context *pcontext)
{
   pcontext->create_vertex_elements_state = grate_create_vertex_state;
   pcontext->bind_vertex_elements_state = grate_bind_vertex_state;
   pcontext->delete_vertex_elements_state = grate_delete_vertex_state;
}
