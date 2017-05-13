#include <stdio.h>

#include "util/u_helpers.h"
#include "util/u_memory.h"

#include "tegra_context.h"
#include "tegra_state.h"

#define unimplemented() printf("TODO: %s()\n", __func__)

static void tegra_set_sample_mask(struct pipe_context *pcontext,
				  unsigned int sample_mask)
{
	unimplemented();
}

static void tegra_set_constant_buffer(struct pipe_context *pcontext,
				      unsigned int shader, unsigned int index,
				      const struct pipe_constant_buffer *buffer)
{
	fprintf(stdout, "> %s(pcontext=%p, shader=%d, index=%d, buffer=%p)\n",
		__func__, pcontext, shader, index, buffer);
	fprintf(stdout, "  buffer:\n");

	if (buffer) {
		fprintf(stdout, "    buffer: %p\n", buffer->buffer);
		fprintf(stdout, "    buffer_offset: %u\n", buffer->buffer_offset);
		fprintf(stdout, "    buffer_size: %u\n", buffer->buffer_size);
		assert(!buffer->user_buffer);
	}

	fprintf(stdout, "< %s()\n", __func__);
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

static void tegra_set_polygon_stipple(struct pipe_context *pcontext,
				      const struct pipe_poly_stipple *stipple)
{
	unimplemented();
}

static void tegra_set_scissor_states(struct pipe_context *pcontext,
                               unsigned start_slot,
                               unsigned num_scissors,
                               const struct pipe_scissor_state * scissors)
{
	assert(num_scissors == 1);
	unimplemented();
}

static void tegra_set_viewport_states(struct pipe_context *pcontext,
                                unsigned start_slot,
                                unsigned num_viewports,
                                const struct pipe_viewport_state *viewports)
{
	assert(num_viewports == 1);
	unimplemented();
}

static void tegra_set_vertex_buffers(struct pipe_context *pcontext,
				     unsigned int start, unsigned int count,
				     const struct pipe_vertex_buffer *buffer)
{
	struct tegra_context *context = tegra_context(pcontext);
	struct tegra_vertexbuf_state *vbs = &context->vbs;
	unsigned int i;

	fprintf(stdout, "> %s(pcontext=%p, start=%u, count=%u, buffer=%p)\n",
		__func__, pcontext, start, count, buffer);
	fprintf(stdout, "  buffers:\n");

	for (i = 0; buffer != NULL && i < count; i++) {
		const struct pipe_vertex_buffer *vb = &buffer[i];

		fprintf(stdout, "    %u:\n", i);
		fprintf(stdout, "      stride: %u\n", vb->stride);
		fprintf(stdout, "      buffer_offset: %u\n", vb->buffer_offset);
		fprintf(stdout, "      buffer: %p\n", vb->buffer);

		assert(!vb->user_buffer);
	}

	util_set_vertex_buffers_mask(vbs->vb, &vbs->enabled, buffer, start,
				     count);
	vbs->count = util_last_bit(vbs->enabled);

	fprintf(stdout, "< %s()\n", __func__);
}


static void tegra_set_sampler_views(struct pipe_context *pcontext,
				    unsigned shader,
				    unsigned start_slot, unsigned num_views,
				    struct pipe_sampler_view **views)
{
	unimplemented();
}

static void tegra_set_index_buffer(struct pipe_context *pcontext,
				   const struct pipe_index_buffer *buffer)
{
	struct tegra_context *context = tegra_context(pcontext);

	fprintf(stdout, "> %s(pcontext=%p, buffer=%p)\n", __func__, pcontext,
		buffer);
	fprintf(stdout, "  buffer:\n");

	if (buffer) {
		fprintf(stdout, "    index_size: %u\n", buffer->index_size);
		fprintf(stdout, "    offset: %u\n", buffer->offset);
		fprintf(stdout, "    buffer: %p\n", buffer->buffer);

		pipe_resource_reference(&context->index_buffer.buffer,
					buffer->buffer);
		context->index_buffer.index_size = buffer->index_size;
		context->index_buffer.offset = buffer->offset;
		assert(!buffer->user_buffer);
	} else {
		pipe_resource_reference(&context->index_buffer.buffer, NULL);
	}

	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_state_init(struct pipe_context *pcontext)
{
	pcontext->set_sample_mask = tegra_set_sample_mask;
	pcontext->set_constant_buffer = tegra_set_constant_buffer;
	pcontext->set_framebuffer_state = tegra_set_framebuffer_state;
	pcontext->set_polygon_stipple = tegra_set_polygon_stipple;
	pcontext->set_scissor_states = tegra_set_scissor_states;
	pcontext->set_viewport_states = tegra_set_viewport_states;
	pcontext->set_sampler_views = tegra_set_sampler_views;
	pcontext->set_vertex_buffers = tegra_set_vertex_buffers;
	pcontext->set_index_buffer = tegra_set_index_buffer;
}

static void *tegra_create_blend_state(struct pipe_context *pcontext,
				      const struct pipe_blend_state *template)
{
	struct tegra_blend_state *so = CALLOC_STRUCT(tegra_blend_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_blend_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_blend_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

void tegra_context_blend_init(struct pipe_context *pcontext)
{
	pcontext->create_blend_state = tegra_create_blend_state;
	pcontext->bind_blend_state = tegra_bind_blend_state;
	pcontext->delete_blend_state = tegra_delete_blend_state;
}

static void *
tegra_create_sampler_state(struct pipe_context *pcontext,
			   const struct pipe_sampler_state *template)
{
	struct tegra_sampler_state *so = CALLOC_STRUCT(tegra_sampler_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_sampler_states(struct pipe_context *pcontext,
                                      unsigned shader, unsigned start_slot,
                                      unsigned num_samplers, void **samplers)
{
	unimplemented();
}

static void tegra_delete_sampler_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

void tegra_context_sampler_init(struct pipe_context *pcontext)
{
	pcontext->create_sampler_state = tegra_create_sampler_state;
	pcontext->bind_sampler_states = tegra_bind_sampler_states;
	pcontext->delete_sampler_state = tegra_delete_sampler_state;
}

static void *
tegra_create_rasterizer_state(struct pipe_context *pcontext,
			      const struct pipe_rasterizer_state *template)
{
	struct tegra_rasterizer_state *so = CALLOC_STRUCT(tegra_rasterizer_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_rasterizer_state(struct pipe_context *pcontext,
					void *so)
{
	unimplemented();
}

static void tegra_delete_rasterizer_state(struct pipe_context *pcontext,
					  void *so)
{
	FREE(so);
}

void tegra_context_rasterizer_init(struct pipe_context *pcontext)
{
	pcontext->create_rasterizer_state = tegra_create_rasterizer_state;
	pcontext->bind_rasterizer_state = tegra_bind_rasterizer_state;
	pcontext->delete_rasterizer_state = tegra_delete_rasterizer_state;
}

static void *
tegra_create_zsa_state(struct pipe_context *pcontext,
		       const struct pipe_depth_stencil_alpha_state *template)
{
	struct tegra_zsa_state *so = CALLOC_STRUCT(tegra_zsa_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_zsa_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_zsa_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

void tegra_context_zsa_init(struct pipe_context *pcontext)
{
	pcontext->create_depth_stencil_alpha_state = tegra_create_zsa_state;
	pcontext->bind_depth_stencil_alpha_state = tegra_bind_zsa_state;
	pcontext->delete_depth_stencil_alpha_state = tegra_delete_zsa_state;
}

static void *
tegra_create_vs_state(struct pipe_context *pcontext,
		      const struct pipe_shader_state *template)
{
	struct tegra_vs_state *so = CALLOC_STRUCT(tegra_vs_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_vs_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_vs_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

void tegra_context_vs_init(struct pipe_context *pcontext)
{
	pcontext->create_vs_state = tegra_create_vs_state;
	pcontext->bind_vs_state = tegra_bind_vs_state;
	pcontext->delete_vs_state = tegra_delete_vs_state;
}

static void *
tegra_create_fs_state(struct pipe_context *pcontext,
		      const struct pipe_shader_state *template)
{
	struct tegra_fs_state *so = CALLOC_STRUCT(tegra_fs_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_fs_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_fs_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

void tegra_context_fs_init(struct pipe_context *pcontext)
{
	pcontext->create_fs_state = tegra_create_fs_state;
	pcontext->bind_fs_state = tegra_bind_fs_state;
	pcontext->delete_fs_state = tegra_delete_fs_state;
}

static void *
tegra_create_vertex_state(struct pipe_context *pcontext, unsigned int count,
			  const struct pipe_vertex_element *elements)
{
	unsigned int i;
	struct tegra_vertex_state *vtx = CALLOC_STRUCT(tegra_vertex_state);
	if (!vtx)
		return NULL;

	fprintf(stdout, "> %s(pcontext=%p, count=%d, elements=%p)\n", __func__, pcontext, count, elements);

	fprintf(stdout, "  elements:\n");
	for (i = 0; i < count; ++i) {
		const struct pipe_vertex_element *e = &elements[i];

		fprintf(stdout, "    %u:\n", i);
		fprintf(stdout, "      src_offset: %u\n", e->src_offset);
		fprintf(stdout, "      instance_divisor: %u\n", e->instance_divisor);
		fprintf(stdout, "      vertex_buffer_index: %u\n", e->vertex_buffer_index);
		fprintf(stdout, "      src_format: %d\n", e->src_format);
	}

	memcpy(vtx->elements, elements, sizeof(*elements) * count);
	vtx->num_elements = count;

	fprintf(stdout, "< %s()\n", __func__);

	return vtx;
}

static void tegra_bind_vertex_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_vertex_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

static void tegra_draw_vbo(struct pipe_context *pcontext,
			   const struct pipe_draw_info *info)
{
	struct tegra_context *context = tegra_context(pcontext);
	struct tegra_channel *gr3d = context->gr3d;

	fprintf(stdout, "> %s(pcontext=%p, info=%p)\n", __func__, pcontext,
		info);
	fprintf(stdout, "  info:\n");
	fprintf(stdout, "    indexed: %d\n", info->indexed);
	fprintf(stdout, "    mode: %u\n", info->mode);
	fprintf(stdout, "    start: %u\n", info->start);
	fprintf(stdout, "    count: %u\n", info->count);

	tegra_stream_begin(&gr3d->stream);
	/* TODO: draw */
	tegra_stream_end(&gr3d->stream);

	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_vbo_init(struct pipe_context *pcontext)
{
	pcontext->create_vertex_elements_state = tegra_create_vertex_state;
	pcontext->bind_vertex_elements_state = tegra_bind_vertex_state;
	pcontext->delete_vertex_elements_state = tegra_delete_vertex_state;
	pcontext->draw_vbo = tegra_draw_vbo;
}
