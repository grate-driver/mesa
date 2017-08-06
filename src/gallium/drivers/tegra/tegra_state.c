#include <stdio.h>

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
   unimplemented();
}

static void
tegra_set_framebuffer_state(struct pipe_context *pcontext,
                            const struct pipe_framebuffer_state *framebuffer)
{
   struct tegra_context *context = tegra_context(pcontext);
   unsigned int i;

   for (i = 0; i < framebuffer->nr_cbufs; i++) {
      struct pipe_surface *ref = framebuffer->cbufs[i];

      if (i >= framebuffer->nr_cbufs)
         ref = NULL;

      pipe_surface_reference(&context->framebuffer.base.cbufs[i],
                   ref);
   }

   pipe_surface_reference(&context->framebuffer.base.zsbuf,
                framebuffer->zsbuf);

   context->framebuffer.base.width = framebuffer->width;
   context->framebuffer.base.height = framebuffer->height;
   context->framebuffer.base.nr_cbufs = framebuffer->nr_cbufs;
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
   assert(num_viewports == 1);
   unimplemented();
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

static void *
tegra_create_rasterizer_state(struct pipe_context *pcontext,
                              const struct pipe_rasterizer_state *template)
{
   struct pipe_rasterizer_state *so = CALLOC_STRUCT(pipe_rasterizer_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
tegra_bind_rasterizer_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
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

static void *
tegra_create_zsa_state(struct pipe_context *pcontext,
                       const struct pipe_depth_stencil_alpha_state *template)
{
   struct pipe_depth_stencil_alpha_state *so = CALLOC_STRUCT(pipe_depth_stencil_alpha_state);
   if (!so)
      return NULL;

   *so = *template;

   return so;
}

static void
tegra_bind_zsa_state(struct pipe_context *pcontext, void *so)
{
   unimplemented();
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

void
tegra_emit_state(struct tegra_context *context)
{
   emit_attribs(context);
}

void
tegra_context_vbo_init(struct pipe_context *pcontext)
{
   pcontext->create_vertex_elements_state = tegra_create_vertex_state;
   pcontext->bind_vertex_elements_state = tegra_bind_vertex_state;
   pcontext->delete_vertex_elements_state = tegra_delete_vertex_state;
}
