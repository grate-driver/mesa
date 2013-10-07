#include <stdio.h>

#include "tegra_context.h"
#include "tegra_resource.h"
#include "tegra_screen.h"

static void tegra_screen_destroy(struct pipe_screen *pscreen)
{
	struct tegra_screen *screen = tegra_screen(pscreen);

	fprintf(stdout, "> %s(pscreen=%p)\n", __func__, pscreen);

	slab_destroy_parent(&screen->transfer_pool);

	drm_tegra_close(screen->drm);
	free(screen);

	fprintf(stdout, "< %s()\n", __func__);
}

static const char *tegra_screen_get_name(struct pipe_screen *pscreen)
{
	const char *name = "Tegra";
	fprintf(stderr, "> %s(pscreen=%p)\n", __func__, pscreen);
	fprintf(stderr, "< %s() = %s\n", __func__, name);
	return name;
}

static const char *tegra_screen_get_vendor(struct pipe_screen *pscreen)
{
	const char *vendor = "NVIDIA";
	fprintf(stderr, "> %s(pscreen=%p)\n", __func__, pscreen);
	fprintf(stderr, "< %s() = %s\n", __func__, vendor);
	return vendor;
}

static int tegra_screen_get_param(struct pipe_screen *pscreen,
				  enum pipe_cap param)
{
	switch (param) {
	case PIPE_CAP_NPOT_TEXTURES:
		return 1; /* not really, but mesa requires it for now! */

	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 0;

	case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
		return 0; /* ??? */

	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 0;

	case PIPE_CAP_POINT_SPRITE:
		return 1;

	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 8; /* ??? */

	case PIPE_CAP_OCCLUSION_QUERY:
		return 0; /* ??? */

	case PIPE_CAP_QUERY_TIME_ELAPSED:
		return 0; /* ??? - can we use syncpts for this? */

	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 0;

	case PIPE_CAP_TEXTURE_SWIZZLE:
		return 0;

	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		return 16;

	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		return 0;

	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 16; /* ??? */

	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
		return 0;

	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		return 1;

	case PIPE_CAP_SM3:
		return 1; /* well, not quite. but perhaps close enough? */

	case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
		return 0;

	case PIPE_CAP_PRIMITIVE_RESTART:
		return 0; /* probably possible to do by splitting draws, but not sure */

	case PIPE_CAP_INDEP_BLEND_ENABLE:
		return 0; /* ??? */

	case PIPE_CAP_INDEP_BLEND_FUNC:
		return 0; /* ??? */

	case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
		return 0;

	case PIPE_CAP_DEPTH_CLIP_DISABLE:
		return 0; /* ??? */

	case PIPE_CAP_SHADER_STENCIL_EXPORT:
		return 0; /* ??? */

	case PIPE_CAP_TGSI_INSTANCEID:
	case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
		return 0;

	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
		return 0; /* probably not */

	case PIPE_CAP_SEAMLESS_CUBE_MAP:
	case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
		return 0; /* probably not */

	case PIPE_CAP_MIN_TEXEL_OFFSET:
	case PIPE_CAP_MAX_TEXEL_OFFSET:
		return 0;

	case PIPE_CAP_CONDITIONAL_RENDER:
		return 0; /* probably not */

	case PIPE_CAP_TEXTURE_BARRIER:
		return 0; /* no clue */

	case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
	case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
	case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
		return 0;

	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
		return 0; /* probably */

	case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
		return 1; /* probably irrelevant for GLES2 */

	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
		return 0; /* probably irrelevant for GLES2 */

	case PIPE_CAP_GLSL_FEATURE_LEVEL:
		return 120; /* no clue */

	case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
		return 0; /* no idea, need to test */

	case PIPE_CAP_USER_VERTEX_BUFFERS:
	case PIPE_CAP_USER_CONSTANT_BUFFERS:
		return 0; /* probably possible, but nasty for kernel */

	case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
		return 0;

	case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
		return 4; /* DWORD aligned, can do pure data GATHER */

	case PIPE_CAP_START_INSTANCE:
		return 0;

	case PIPE_CAP_QUERY_TIMESTAMP:
		return 0; /* dunno */

	case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
		return 0;

	case PIPE_CAP_CUBE_MAP_ARRAY:
		return 0;

	case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
		return 1; /* what is this? */

	case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
		return 1; /* what is this? */

	case PIPE_CAP_TGSI_TEXCOORD:
		return 0;

	case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
		return 1;

	default:
		fprintf(stdout, "  unsupported parameter: %d\n", param);
		return 0;
	}
}

static float tegra_screen_get_paramf(struct pipe_screen *pscreen,
				     enum pipe_capf param)
{
	switch (param) {
	case PIPE_CAPF_MAX_LINE_WIDTH:
	case PIPE_CAPF_MAX_LINE_WIDTH_AA:
	case PIPE_CAPF_MAX_POINT_WIDTH:
	case PIPE_CAPF_MAX_POINT_WIDTH_AA:
		return 8192.0f; /* no clue */

	case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
		return 0.0f;

	case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
		return 16.0f;

	default:
		fprintf(stdout, "  unsupported parameter: %d\n", param);
		return 0.0f;
	}
}

static int tegra_screen_get_shader_param(struct pipe_screen *pscreen,
					 unsigned int shader,
					 enum pipe_shader_cap param)
{
	switch (shader) {
	case PIPE_SHADER_VERTEX:
		switch (param) {

		case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
			return 1024;

		/* no vertex-texturing */
		case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
		case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
			return 0;

		case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
		case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		case PIPE_SHADER_CAP_SUBROUTINES:
			return 0;

		case PIPE_SHADER_CAP_MAX_INPUTS:
		case PIPE_SHADER_CAP_MAX_OUTPUTS:
			return 16;

		case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
		case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
			return 1024;

		case PIPE_SHADER_CAP_MAX_TEMPS:
			return 64 * 4; /* 64 vec4s */

		/* cannot index attributes, varyings nor GPRs */
		case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
			return 0;

		case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
			return 1; /* can index constant registers */

		case PIPE_SHADER_CAP_INTEGERS:
			return 0;

		case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
			return 1;

		default:
			fprintf(stdout, "unsupported vertex-shader parameter: %d\n", param);
			return 0;
		}

	case PIPE_SHADER_FRAGMENT:

		switch (param) {
		case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
			return 4 * 128;

		case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
			return 4 * 128;

		case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
			return 128;

		case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
			return 128;

		/* no control flow */
		case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
		case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		case PIPE_SHADER_CAP_SUBROUTINES:
			return 0;

		case PIPE_SHADER_CAP_MAX_INPUTS:
		case PIPE_SHADER_CAP_MAX_OUTPUTS:
			return 16;

		case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
		case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
			return 32;

		case PIPE_SHADER_CAP_MAX_TEMPS:
			return 16; /* scalars */

		/* no indirection */
		case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
			return 0;

		case PIPE_SHADER_CAP_INTEGERS:
			return 0;

		case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
			return 16;

		case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
			return 1;

		default:
			fprintf(stdout, "unsupported parameter: %d\n", param);
			return 0;
		}
		break;

	case PIPE_SHADER_GEOMETRY:
		fprintf(stdout, "  PIPE_SHADER_GEOMETRY not supported\n");
		return 0;

	case PIPE_SHADER_COMPUTE:
		fprintf(stdout, "  PIPE_SHADER_COMPUTE not supported\n");
		return 0;

	default:
		fprintf(stdout, "  unknown shader type: %u\n", shader);
		return 0;
	}
}

static boolean tegra_screen_is_format_supported(struct pipe_screen *pscreen,
						enum pipe_format format,
						enum pipe_texture_target target,
						unsigned int sample_count,
						unsigned int bindings)
{
	switch (format) {
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return TRUE;
	default:
		return FALSE;
	}
}

static void
tegra_screen_fence_reference(struct pipe_screen *pscreen,
			struct pipe_fence_handle **ptr,
			struct pipe_fence_handle *fence)
{
	fprintf(stdout, "> %s\n", __func__);

// 	assert(ptr && *ptr);
// 	assert(fence);
//
// 	if (pipe_reference(&tegra_fence(*ptr)->reference, &tegra_fence(fence)->reference)) {
// 		free(tegra_fence(*ptr)->fence);
// 		free(*ptr);
// 	}
//
// 	*ptr = fence;
	fprintf(stdout, "< %s()\n", __func__);
}

static boolean
tegra_screen_fence_finish(struct pipe_screen *screen,
		struct pipe_context *ctx,
		struct pipe_fence_handle *fence,
		uint64_t timeout)
{
	boolean ret = 0;

	fprintf(stdout, "> %s\n", __func__);

	fprintf(stdout, "< %s() = %d\n", __func__, ret);

	return ret;
}

struct pipe_screen *tegra_screen_create(struct drm_tegra *drm)
{
	struct tegra_screen *screen;

	fprintf(stdout, "> %s(drm=%p)\n", __func__, drm);

	screen = calloc(1, sizeof(*screen));
	if (!screen) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	screen->drm = drm;

	screen->base.destroy = tegra_screen_destroy;
	screen->base.get_name = tegra_screen_get_name;
	screen->base.get_vendor = tegra_screen_get_vendor;
	screen->base.get_param = tegra_screen_get_param;
	screen->base.get_paramf = tegra_screen_get_paramf;
	screen->base.get_shader_param = tegra_screen_get_shader_param;
	screen->base.context_create = tegra_screen_context_create;
	screen->base.is_format_supported = tegra_screen_is_format_supported;

	/* fence functions */
	screen->base.fence_reference = tegra_screen_fence_reference;
	screen->base.fence_finish = tegra_screen_fence_finish;

	tegra_screen_resource_init(&screen->base);

	slab_create_parent(&screen->transfer_pool, sizeof(struct pipe_transfer), 16);

	fprintf(stdout, "< %s() = %p\n", __func__, &screen->base);
	return &screen->base;
}
