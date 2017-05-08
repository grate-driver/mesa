#include <stdio.h>

#include "util/u_helpers.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_info.h"

#include "tegra_context.h"
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
	struct tegra_context *context = tegra_context(pcontext);
	unsigned int i;

	fprintf(stdout, "> %s(pcontext=%p, framebuffer=%p)\n", __func__,
		pcontext, framebuffer);
	fprintf(stdout, "  framebuffer:\n");
	fprintf(stdout, "    resolution: %ux%u\n", framebuffer->width,
		framebuffer->height);
	fprintf(stdout, "    nr_cbufs: %u\n", framebuffer->nr_cbufs);

	for (i = 0; i < framebuffer->nr_cbufs; i++)
		fprintf(stdout, "      %u: %p\n", i, framebuffer->cbufs[i]);

	fprintf(stdout, "    zsbuf: %p\n", framebuffer->zsbuf);

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

	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_polygon_stipple(struct pipe_context *pcontext,
				      const struct pipe_poly_stipple *stipple)
{
	fprintf(stdout, "> %s(pcontext=%p, stipple=%p)\n", __func__, pcontext,
		stipple);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_scissor_states(struct pipe_context *pcontext,
                               unsigned start_slot,
                               unsigned num_scissors,
                               const struct pipe_scissor_state * scissors)
{
	fprintf(stdout, "> %s(pcontext=%p, start_slot=%d, num_scissors=%d, scissors=%p)\n",
		__func__, pcontext, start_slot, num_scissors, scissors);
	assert(num_scissors == 1);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_set_viewport_states(struct pipe_context *pcontext,
                                unsigned start_slot,
                                unsigned num_viewports,
                                const struct pipe_viewport_state *viewports)
{
	fprintf(stdout, "> %s(pcontext=%p, start_slot=%d, num_viewports=%d, viewports=%p)\n",
		__func__, pcontext, start_slot, num_viewports, viewports);
	assert(num_viewports == 1);
	fprintf(stdout, "< %s()\n", __func__);
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
	fprintf(stdout, "> %s(pcontext=%p, shader=%u, start_slot=%u num_views=%u, views=%p)\n",
		__func__, pcontext, shader, start_slot, num_views, views);
	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_blend_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = CALLOC_STRUCT(tegra_blend_state);
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

	FREE(so);

	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_sampler_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = CALLOC_STRUCT(tegra_sampler_state);
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	so->base = *template;

	fprintf(stdout, "< %s() = %p\n", __func__, so);
	return so;
}

static void tegra_bind_sampler_states(struct pipe_context *pcontext,
                                      unsigned shader, unsigned start_slot,
                                      unsigned num_samplers, void **samplers)
{
	fprintf(stdout, "> %s(pcontext=%p, shader=%u, start_slot=%u, num_samplerst=%u, samplers=%p)\n", __func__,
		pcontext, shader, start_slot, num_samplers, samplers);
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_sampler_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	FREE(so);

	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_rasterizer_state *so;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = CALLOC_STRUCT(tegra_rasterizer_state);
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

	FREE(so);

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

	so = CALLOC_STRUCT(tegra_zsa_state);
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

	FREE(so);

	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_zsa_init(struct pipe_context *pcontext)
{
	pcontext->create_depth_stencil_alpha_state = tegra_create_zsa_state;
	pcontext->bind_depth_stencil_alpha_state = tegra_bind_zsa_state;
	pcontext->delete_depth_stencil_alpha_state = tegra_delete_zsa_state;
}

static void print_dst_reg(const struct tgsi_dst_register *dst)
{
	fprintf(stdout, " %c%d.%s%s%s%s",
	    dst->File == TGSI_FILE_OUTPUT ? 'o' : 'r',
	    dst->Index,
	    (dst->WriteMask & TGSI_WRITEMASK_X) ? "x" : "",
	    (dst->WriteMask & TGSI_WRITEMASK_Y) ? "y" : "",
	    (dst->WriteMask & TGSI_WRITEMASK_Z) ? "z" : "",
	    (dst->WriteMask & TGSI_WRITEMASK_W) ? "w" : "");
}

static void print_src_reg(const struct tgsi_src_register *src)
{
	const char swz[] = { 'x', 'y', 'z', 'w'};
	char ch = 'r';
	switch (src->File) {
	case TGSI_FILE_INPUT:
		ch = 'a';
		break;
	case TGSI_FILE_CONSTANT:
		ch = 'u';
		break;
	}

	fprintf(stdout, ", %c%d.%c%c%c%c", ch, src->Index,
	    swz[src->SwizzleX],
	    swz[src->SwizzleY],
	    swz[src->SwizzleZ],
	    swz[src->SwizzleW]);
}


static void emit_vs_instr(struct tegra_vs_state *so, const struct tgsi_full_instruction *inst)
{
	fprintf(stdout, "instr: %s", tgsi_get_opcode_name(inst->Instruction.Opcode));

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_END:
		break;

	case TGSI_OPCODE_MOV:
		print_dst_reg(&inst->Dst[0].Register);
		print_src_reg(&inst->Src[0].Register);
		break;

	case TGSI_OPCODE_MUL:
	case TGSI_OPCODE_DP4:
		print_dst_reg(&inst->Dst[0].Register);
		print_src_reg(&inst->Src[0].Register);
		print_src_reg(&inst->Src[1].Register);
		break;

	case TGSI_OPCODE_MAD:
		print_dst_reg(&inst->Dst[0].Register);
		print_src_reg(&inst->Src[0].Register);
		print_src_reg(&inst->Src[1].Register);
		print_src_reg(&inst->Src[2].Register);
		break;

	default:
		assert(0); /* unhandled opcode */
	}
	fprintf(stdout, "\n");
}

static void *
tegra_create_vs_state(struct pipe_context *pcontext,
		      const struct pipe_shader_state *template)
{
	unsigned ok;
	struct tegra_vs_state *so;
	struct tgsi_parse_context parser;

	fprintf(stdout, "> %s(pcontext=%p, template=%p)\n", __func__,
		pcontext, template);

	so = CALLOC_STRUCT(tegra_vs_state);
	if (!so) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	/* TODO: ditch this, we don't need it */
	so->base = *template;

	ok = tgsi_parse_init(&parser, template->tokens);

	/* we shouldn't get malformed programs at this point, no? */
	assert(ok == TGSI_PARSE_OK);

	/* yeah, I'm being paranoid! */
	assert(parser.FullHeader.Processor.Processor == PIPE_SHADER_VERTEX);

	while (!tgsi_parse_end_of_tokens(&parser)) {
		const struct tgsi_full_declaration *decl;
		const struct tgsi_full_immediate *imm;
		const struct tgsi_full_instruction *inst;

	        tgsi_parse_token(&parser);

		switch (parser.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION:
			decl = &parser.FullToken.FullDeclaration;

			/* we need to consider the semantics */
			assert(decl->Declaration.Semantic);

			switch (decl->Declaration.File) {
			case TGSI_FILE_INPUT:
				/* OpenGL ES 2.0 only supports generic attributes */
				assert(decl->Semantic.Name == TGSI_SEMANTIC_GENERIC);
				fprintf(stdout, "input-generic %d (%d)\n", decl->Semantic.Index, decl->Range.First);
				break;

			case TGSI_FILE_OUTPUT:
				switch (decl->Semantic.Name) {
				case TGSI_SEMANTIC_POSITION:
					fprintf(stdout, "position (%d)\n", decl->Range.First);
					assert(decl->Semantic.Index == 0);
					break;
				case TGSI_SEMANTIC_PSIZE:
					fprintf(stdout, "point-size (%d)\n", decl->Range.First);
					assert(decl->Semantic.Index == 0);
					break;

				case TGSI_SEMANTIC_GENERIC:
					fprintf(stdout, "output-generic %d (%d)\n", decl->Semantic.Index, decl->Range.First);
					break;

				default:
					assert(0); /* unsupported output-semantic */
				}
				break;

			case TGSI_FILE_CONSTANT:
				/* declares uniform */
				break;

			default:
				assert(0); /* unsupported register-file */
			}
			break;

		case TGSI_TOKEN_TYPE_IMMEDIATE:
			imm = &parser.FullToken.FullImmediate;

			/* no integers for now */
			assert(imm->Immediate.Type == TGSI_IMM_FLOAT32);

			fprintf(stdout, "imm: %f %f %f %f\n",
			    imm->u[0].Float, imm->u[1].Float,
			    imm->u[2].Float, imm->u[3].Float);

			break;

		case TGSI_TOKEN_TYPE_INSTRUCTION:
			inst = &parser.FullToken.FullInstruction;
			emit_vs_instr(so, inst);
			break;

		case TGSI_TOKEN_TYPE_PROPERTY:
			break;

		default:
			assert(0); /* unsupported token */
		}

	}

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

	FREE(so);

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

	so = CALLOC_STRUCT(tegra_fs_state);
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

	FREE(so);

	fprintf(stdout, "< %s()\n", __func__);
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
	struct tegra_vertex_state *vtx;

	fprintf(stdout, "> %s(pcontext=%p, count=%u, elements=%p)\n",
		__func__, pcontext, count, elements);

	vtx = CALLOC_STRUCT(tegra_vertex_state);
	if (!vtx) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	memcpy(vtx->elements, elements, sizeof(*elements) * count);
	vtx->num_elements = count;

	fprintf(stdout, "< %s() = %p\n", __func__, vtx);
	return vtx;
}

static void tegra_bind_vertex_state(struct pipe_context *pcontext, void *so)
{
	struct tegra_vertex_state *vs = so;
	unsigned int i;

	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);
	if (vs) {
		fprintf(stdout, "  vs:\n");
		fprintf(stdout, "    num_elements: %u\n", vs->num_elements);

		for (i = 0; i < vs->num_elements; i++) {
			struct pipe_vertex_element *element = &vs->elements[i];

			fprintf(stdout, "      %u:\n", i);
			fprintf(stdout, "        src_offset: %u\n", element->src_offset);
			fprintf(stdout, "        instance_divisor: %u\n", element->instance_divisor);
			fprintf(stdout, "        vertex_buffer_index: %u\n", element->vertex_buffer_index);
			fprintf(stdout, "        src_format: %d\n", element->src_format);
		}
	}
	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_delete_vertex_state(struct pipe_context *pcontext, void *so)
{
	fprintf(stdout, "> %s(pcontext=%p, so=%p)\n", __func__, pcontext, so);

	FREE(so);

	fprintf(stdout, "< %s()\n", __func__);
}

static void tegra_draw_vbo(struct pipe_context *pcontext,
			   const struct pipe_draw_info *info)
{
	fprintf(stdout, "> %s(pcontext=%p, info=%p)\n", __func__, pcontext,
		info);
	fprintf(stdout, "  info:\n");
	fprintf(stdout, "    indexed: %d\n", info->indexed);
	fprintf(stdout, "    mode: %u\n", info->mode);
	fprintf(stdout, "    start: %u\n", info->start);
	fprintf(stdout, "    count: %u\n", info->count);

	/* TODO: draw */

	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_context_vbo_init(struct pipe_context *pcontext)
{
	pcontext->create_vertex_elements_state = tegra_create_vertex_state;
	pcontext->bind_vertex_elements_state = tegra_bind_vertex_state;
	pcontext->delete_vertex_elements_state = tegra_delete_vertex_state;
	pcontext->draw_vbo = tegra_draw_vbo;
}
