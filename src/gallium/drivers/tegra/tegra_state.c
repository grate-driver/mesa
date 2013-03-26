#include <stdio.h>

#include "tegra_state.h"

static void tegra_set_sample_mask(struct pipe_context *pcontext,
				  unsigned int sample_mask)
{
	fprintf(stdout, "> %s(pcontext=%p, sample_mask=%x)\n", __func__,
		pcontext, sample_mask);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_constant_buffer(struct pipe_context *pcontext,
				      unsigned int shader, unsigned int index,
				      const struct pipe_constant_buffer *buffer)
{
	fprintf(stdout, "> %s(pcontext=%p, shader=%u, index=%u, buffer=%p)\n",
		__func__, pcontext, shader, index, buffer);
	fprintf(stdout, "< %s()\n", __func__);
}

static void
tegra_set_framebuffer_state(struct pipe_context *pcontext,
			    const struct pipe_framebuffer_state *framebuffer)
{
	fprintf(stdout, "> %s(pcontext=%p, framebuffer=%p)\n", __func__,
		pcontext, framebuffer);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_polygon_stipple(struct pipe_context *pcontext,
				      const struct pipe_poly_stipple *stipple)
{
	fprintf(stdout, "> %s(pcontext=%p, stipple=%p)\n", __func__, pcontext,
		stipple);
	fprintf(stdout, "< %s()\n", __func__);
}

static void
tegra_set_viewport_states(struct pipe_context *pcontext,
			  unsigned start_slot,
			  unsigned num_viewports,
			  const struct pipe_viewport_state *viewport)
{
	fprintf(stdout, "> %s(pcontext=%p, start_slot=%u num_viewports=%u viewport=%p)\n",
		__func__, pcontext, start_slot, num_viewports, viewport);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_vertex_buffers(struct pipe_context *pcontext,
				     unsigned int start, unsigned int count,
				     const struct pipe_vertex_buffer *buffer)
{
	fprintf(stdout, "> %s(pcontext=%p, start=%u, count=%u, buffer=%p)\n",
		__func__, pcontext, start, count, buffer);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_index_buffer(struct pipe_context *pcontext,
				   const struct pipe_index_buffer *buffer)
{
	fprintf(stdout, "> %s(pcontext=%p, buffer=%p)\n", __func__, pcontext,
		buffer);
	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_state_init(struct pipe_context *pcontext)
{
	pcontext->set_sample_mask = tegra_set_sample_mask;
	pcontext->set_constant_buffer = tegra_set_constant_buffer;
	pcontext->set_framebuffer_state = tegra_set_framebuffer_state;
	pcontext->set_polygon_stipple = tegra_set_polygon_stipple;
	pcontext->set_viewport_states = tegra_set_viewport_states;
	pcontext->set_vertex_buffers = tegra_set_vertex_buffers;
	pcontext->set_index_buffer = tegra_set_index_buffer;
}

static void *tegra_create_blend_state(struct pipe_context *pcontext,
				      const struct pipe_blend_state *template)
{
	struct tegra_blend_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = calloc(1, sizeof(*so));
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	so->base = *template;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_blend_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_blend_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	free(so);

	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_blend_init(struct pipe_context *pcontext)
{
	pcontext->create_blend_state = tegra_create_blend_state;
	pcontext->bind_blend_state = tegra_bind_blend_state;
	pcontext->delete_blend_state = tegra_delete_blend_state;
}

static void *
tegra_create_rasterizer_state(struct pipe_context *pcontext,
			      const struct pipe_rasterizer_state *template)
{
	struct tegra_rasterizer_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = calloc(1, sizeof(*so));
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	so->base = *template;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_rasterizer_state(struct pipe_context *pcontext,
					void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_rasterizer_state(struct pipe_context *pcontext,
					  void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	free(so);

	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_zsa_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = calloc(1, sizeof(*so));
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	so->base = *template;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_zsa_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_zsa_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	free(so);

	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_vs_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = calloc(1, sizeof(*so));
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	so->base = *template;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_vs_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_vs_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	free(so);

	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_fs_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = calloc(1, sizeof(*so));
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	so->base = *template;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_fs_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_fs_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	free(so);

	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_fs_init(struct pipe_context *pcontext)
{
	pcontext->create_fs_state = tegra_create_fs_state;
	pcontext->bind_fs_state = tegra_bind_fs_state;
	pcontext->delete_fs_state = tegra_delete_fs_state;
}

static void *
tegra_create_vbo_state(struct pipe_context *pcontext, unsigned int count,
		       const struct pipe_vertex_element *elements)
{
	struct tegra_vbo_state *so;

	fprintf(stdout, "> %s(pcontext=%p, count=%u, elements=%p)\n",
		__func__, pcontext, count, elements);

	so = calloc(1, sizeof(*so));
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	memcpy(so->elements, elements, sizeof(*elements) * count);
	so->num_elements = count;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_vbo_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_vbo_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	free(so);

	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_draw_vbo(struct pipe_context *pcontext,
			   const struct pipe_draw_info *info)
{
	fprintf(stdout, "> %s(pcontext=%p, info=%p)\n", __func__, pcontext,
		info);
	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_vbo_init(struct pipe_context *pcontext)
{
	pcontext->create_vertex_elements_state = tegra_create_vbo_state;
	pcontext->bind_vertex_elements_state = tegra_bind_vbo_state;
	pcontext->delete_vertex_elements_state = tegra_delete_vbo_state;
	pcontext->draw_vbo = tegra_draw_vbo;
}
