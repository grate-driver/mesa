#include <stdio.h>

#include "util/u_helpers.h"
#include "util/u_memory.h"
#include "util/u_format.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_info.h"

#include "tegra_context.h"
#include "tegra_resource.h"
#include "tegra_state.h"

#include "tgr_3d.xml.h"

#include "host1x01_hardware.h"

#define unimplemented() printf("TODO: %s()\n", __func__)

#define TGR3D_VAL(reg_name, field_name, value) \
	(((value) << TGR3D_ ## reg_name ## _ ## field_name ## __SHIFT) & \
		     TGR3D_ ## reg_name ## _ ## field_name ## __MASK)


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
	int i;
	fprintf(stdout, "instr: %s", tgsi_get_opcode_name(inst->Instruction.Opcode));

	for (i = 0; i < inst->Instruction.NumDstRegs; ++i)
		print_dst_reg(&inst->Dst[i].Register);

	for (i = 0; i < inst->Instruction.NumSrcRegs; ++i)
		print_src_reg(&inst->Src[i].Register);

	fprintf(stdout, "\n");
}

static void *
tegra_create_vs_state(struct pipe_context *pcontext,
		      const struct pipe_shader_state *template)
{
	unsigned ok;
	struct tgsi_parse_context parser;

	struct tegra_vs_state *so = CALLOC_STRUCT(tegra_vs_state);
	if (!so)
		return NULL;

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

			switch (decl->Declaration.File) {
			case TGSI_FILE_INPUT:
				/* OpenGL ES 2.0 only supports generic attributes */
				if (decl->Declaration.Semantic) {
					assert(decl->Semantic.Name == TGSI_SEMANTIC_GENERIC);
					fprintf(stdout, "input-generic %d (%d)\n", decl->Semantic.Index, decl->Range.First);
				} else
					fprintf(stdout, "input-generic\n");
				break;

			case TGSI_FILE_OUTPUT:
				switch (decl->Semantic.Name) {
				case TGSI_SEMANTIC_POSITION:
					fprintf(stdout, "position (%d)\n", decl->Range.First);
					assert(decl->Declaration.Semantic);
					assert(decl->Semantic.Index == 0);
					break;

				case TGSI_SEMANTIC_COLOR:
					fprintf(stdout, "color (%d)\n", decl->Range.First);
					assert(decl->Declaration.Semantic);
					assert(decl->Semantic.Index == 0);
					break;

				case TGSI_SEMANTIC_PSIZE:
					fprintf(stdout, "point-size (%d)\n", decl->Range.First);
					assert(decl->Declaration.Semantic);
					assert(decl->Semantic.Index == 0);
					break;

				case TGSI_SEMANTIC_GENERIC:
					fprintf(stdout, "output-generic %d (%d)\n", decl->Semantic.Index, decl->Range.First);
					assert(decl->Declaration.Semantic);
					break;

				case TGSI_SEMANTIC_FOG:
					fprintf(stdout, "fog\n");
					break;

				default:
					assert(0); /* unsupported output-semantic */
				}
				break;

			case TGSI_FILE_CONSTANT:
				fprintf(stdout, "const\n");
				break;

			case TGSI_FILE_TEMPORARY:
				fprintf(stdout, "temp\n");
				break;

			default:
				assert(0); /* unsupported register-file */
			}
			break;

		case TGSI_TOKEN_TYPE_IMMEDIATE:
			imm = &parser.FullToken.FullImmediate;

			switch (imm->Immediate.Type) {
			case TGSI_IMM_FLOAT32:
				fprintf(stdout, "imm: %f %f %f %f\n",
				    imm->u[0].Float, imm->u[1].Float,
				    imm->u[2].Float, imm->u[3].Float);
				break;

			case TGSI_IMM_UINT32:
				fprintf(stdout, "imm: %d %d %d %d\n",
				    imm->u[0].Uint, imm->u[1].Uint,
				    imm->u[2].Uint, imm->u[3].Uint);
				break;

			default:
				assert(0);
			}
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

/*
 * Note: this does not include the stride, which needs to be mixed in later
 **/
static uint32_t tegra_attrib_mode(const struct pipe_vertex_element *e)
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

static void tegra_bind_vertex_state(struct pipe_context *pcontext, void *so)
{
	tegra_context(pcontext)->vs = so;
}

static void tegra_delete_vertex_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

static void setup_attribs(struct tegra_context *context)
{
	unsigned int i;
	struct tegra_stream *stream = &context->gr3d->stream;

	assert(context->vs);

	for (i = 0; i < 16; ++i) {
		const struct pipe_vertex_buffer *vb;
		const struct tegra_vertex_element *e = context->vs->elements + i;
		const struct tegra_resource *r;

		assert(e->buffer_index < context->vbs.count);
		vb = context->vbs.vb + e->buffer_index;
		r = tegra_resource(vb->buffer);

		uint32_t attrib = e->attrib;
		attrib |= TGR3D_VAL(ATTRIB_MODE, STRIDE, vb->stride);

		tegra_stream_push(stream, host1x_opcode_incr(TGR3D_ATTRIB_PTR(i), 2));
		tegra_stream_push_reloc(stream, r->bo, e->offset);
		tegra_stream_push(stream, attrib);
	}
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

	setup_attribs(context);

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
