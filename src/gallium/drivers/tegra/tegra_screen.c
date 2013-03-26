#include <stdio.h>

#include "tegra_context.h"
#include "tegra_resource.h"
#include "tegra_screen.h"

static void tegra_screen_destroy(struct pipe_screen *pscreen)
{
	fprintf(stdout, "> %s(pscreen=%p)\n", __func__, pscreen);
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
	int ret = 0;

	//fprintf(stdout, "> %s(pscreen=%p, param=%d)\n", __func__, pscreen,
	//	param);

	switch (param) {
	case PIPE_CAP_NPOT_TEXTURES:
		fprintf(stdout, "  PIPE_CAP_NPOT_TEXTURES\n");
		ret = 1;
		break;

	case PIPE_CAP_TWO_SIDED_STENCIL:
		fprintf(stdout, "  PIPE_CAP_TWO_SIDED_STENCIL\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
		fprintf(stdout, "  PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS\n");
		ret = 1;
		break;

	case PIPE_CAP_ANISOTROPIC_FILTER:
		fprintf(stdout, "  PIPE_CAP_ANISOTROPIC_FILTER\n");
		ret = 1;
		break;

	case PIPE_CAP_POINT_SPRITE:
		fprintf(stdout, "  PIPE_CAP_POINT_SPRITE\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_RENDER_TARGETS:
		fprintf(stdout, "  PIPE_CAP_MAX_RENDER_TARGETS\n");
		ret = 8;
		break;

	case PIPE_CAP_OCCLUSION_QUERY:
		fprintf(stdout, "  PIPE_CAP_OCCLUSION_QUERY\n");
		ret = 0;
		break;

	case PIPE_CAP_QUERY_TIME_ELAPSED:
		fprintf(stdout, "  PIPE_CAP_QUERY_TIME_ELAPSED\n");
		ret = 0;
		break;

	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		fprintf(stdout, "  PIPE_CAP_TEXTURE_SHADOW_MAP\n");
		ret = 1;
		break;

	case PIPE_CAP_TEXTURE_SWIZZLE:
		fprintf(stdout, "  PIPE_CAP_TEXTURE_SWIZZLE\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		fprintf(stdout, "  PIPE_CAP_MAX_TEXTURE_2D_LEVELS\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		fprintf(stdout, "  PIPE_CAP_MAX_TEXTURE_3D_LEVELS\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		fprintf(stdout, "  PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS\n");
		ret = 1;
		break;

	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
		fprintf(stdout, "  PIPE_CAP_TEXTURE_MIRROR_CLAMP\n");
		ret = 1;
		break;

	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		fprintf(stdout, "  PIPE_CAP_BLEND_EQUATION_SEPARATE\n");
		ret = 1;
		break;

	case PIPE_CAP_SM3:
		fprintf(stdout, "  PIPE_CAP_SM3\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
		fprintf(stdout, "  PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS\n");
		ret = 0;
		break;

	case PIPE_CAP_PRIMITIVE_RESTART:
		fprintf(stdout, "  PIPE_CAP_PRIMITIVE_RESTART\n");
		ret = 1;
		break;

	case PIPE_CAP_INDEP_BLEND_ENABLE:
		fprintf(stdout, "  PIPE_CAP_INDEP_BLEND_ENABLE\n");
		ret = 0;
		break;

	case PIPE_CAP_INDEP_BLEND_FUNC:
		fprintf(stdout, "  PIPE_CAP_INDEP_BLEND_FUNC\n");
		ret = 0;
		break;

	case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
		fprintf(stdout, "  PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS\n");
		ret = 1;
		break;

	case PIPE_CAP_DEPTH_CLIP_DISABLE:
		fprintf(stdout, "  PIPE_CAP_DEPTH_CLIP_DISABLE\n");
		ret = 0;
		break;

	case PIPE_CAP_SHADER_STENCIL_EXPORT:
		fprintf(stdout, "  PIPE_CAP_SHADER_STENCIL_EXPORT\n");
		ret = 1;
		break;

	case PIPE_CAP_TGSI_INSTANCEID:
		fprintf(stdout, "  PIPE_CAP_TGSI_INSTANCEID\n");
		ret = 1;
		break;

	case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
		fprintf(stdout, "  PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR\n");
		ret = 1;
		break;

	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
		fprintf(stdout, "  PIPE_CAP_FRAGMENT_COLOR_CLAMPED\n");
		ret = 0;
		break;

	case PIPE_CAP_SEAMLESS_CUBE_MAP:
		fprintf(stdout, "  PIPE_CAP_SEAMLESS_CUBE_MAP\n");
		ret = 1;
		break;

	case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
		fprintf(stdout, "  PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE\n");
		ret = 0;
		break;

	case PIPE_CAP_MIN_TEXEL_OFFSET:
		fprintf(stdout, "  PIPE_CAP_MIN_TEXEL_OFFSET\n");
		ret = 0;
		break;

	case PIPE_CAP_MAX_TEXEL_OFFSET:
		fprintf(stdout, "  PIPE_CAP_MAX_TEXEL_OFFSET\n");
		ret = 0;
		break;

	case PIPE_CAP_CONDITIONAL_RENDER:
		fprintf(stdout, "  PIPE_CAP_CONDITIONAL_RENDER\n");
		ret = 1;
		break;

	case PIPE_CAP_TEXTURE_BARRIER:
		fprintf(stdout, "  PIPE_CAP_TEXTURE_BARRIER\n");
		ret = 1;
		break;

	case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
		fprintf(stdout, "  PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS\n");
		ret = 0;
		break;

	case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
		fprintf(stdout, "  PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS\n");
		ret = 0;
		break;

	case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
		fprintf(stdout, "  PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME\n");
		ret = 0;
		break;

	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
		fprintf(stdout, "  PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS\n");
		ret = 0;
		break;

	case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
		fprintf(stdout, "  PIPE_CAP_VERTEX_COLOR_UNCLAMPED\n");
		ret = 1;
		break;

	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
		fprintf(stdout, "  PIPE_CAP_VERTEX_COLOR_CLAMPED\n");
		ret = 0;
		break;

	case PIPE_CAP_GLSL_FEATURE_LEVEL:
		fprintf(stdout, "  PIPE_CAP_GLSL_FEATURE_LEVEL\n");
		ret = 120;
		break;

	case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
		fprintf(stdout, "  PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION\n");
		ret = 1;
		break;

	case PIPE_CAP_USER_VERTEX_BUFFERS:
		fprintf(stdout, "  PIPE_CAP_USER_VERTEX_BUFFERS\n");
		ret = 1;
		break;

	case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
		fprintf(stdout, "  PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY\n");
		ret = 0;
		break;

	case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
		fprintf(stdout, "  PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY\n");
		ret = 0;
		break;

	case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
		fprintf(stdout, "  PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY\n");
		ret = 0;
		break;

	case PIPE_CAP_USER_CONSTANT_BUFFERS:
		fprintf(stdout, "  PIPE_CAP_USER_CONSTANT_BUFFERS\n");
		ret = 1;
		break;

	case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
		fprintf(stdout, "  PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT\n");
		ret = 64;
		break;

	case PIPE_CAP_START_INSTANCE:
		fprintf(stdout, "  PIPE_CAP_START_INSTANCE\n");
		ret = 1;
		break;

	case PIPE_CAP_QUERY_TIMESTAMP:
		fprintf(stdout, "  PIPE_CAP_QUERY_TIMESTAMP\n");
		ret = 0;
		break;

	case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
		fprintf(stdout, "  PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT\n");
		ret = 1;
		break;

	case PIPE_CAP_CUBE_MAP_ARRAY:
		fprintf(stdout, "  PIPE_CAP_CUBE_MAP_ARRAY\n");
		ret = 0;
		break;

	case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
		fprintf(stdout, "  PIPE_CAP_TEXTURE_BUFFER_OBJECTS\n");
		ret = 1;
		break;

	case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
		fprintf(stdout, "  PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT\n");
		ret = 64;
		break;

	case PIPE_CAP_TGSI_TEXCOORD:
		fprintf(stdout, "  PIPE_CAP_TGSI_TEXCOORD\n");
		ret = 0;
		break;

	case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
		fprintf(stdout, "  PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER\n");
		ret = 0;
		break;

	default:
		fprintf(stdout, "  unsupported parameter: %d\n", param);
		break;
	}

	//fprintf(stdout, "< %s() = %d\n", __func__, ret);
	return ret;
}

static float tegra_screen_get_paramf(struct pipe_screen *pscreen,
				     enum pipe_capf param)
{
	float ret = 0.0f;

	//fprintf(stdout, "> %s(pscreen=%p, param=%d)\n", __func__, pscreen,
	//	param);

	switch (param) {
	case PIPE_CAPF_MAX_LINE_WIDTH:
		fprintf(stdout, "  PIPE_CAPF_MAX_LINE_WIDTH\n");
		ret = 8192.0f;
		break;

	case PIPE_CAPF_MAX_LINE_WIDTH_AA:
		fprintf(stdout, "  PIPE_CAPF_MAX_LINE_WIDTH_AA\n");
		ret = 8192.0f;
		break;

	case PIPE_CAPF_MAX_POINT_WIDTH:
		fprintf(stdout, "  PIPE_CAPF_MAX_POINT_WIDTH\n");
		ret = 8192.0f;
		break;

	case PIPE_CAPF_MAX_POINT_WIDTH_AA:
		fprintf(stdout, "  PIPE_CAPF_MAX_POINT_WIDTH_AA\n");
		ret = 8192.0f;
		break;

	case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
		fprintf(stdout, "  PIPE_CAPF_MAX_TEXTURE_ANISOTROPY\n");
		ret = 16.0f;
		break;

	case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
		fprintf(stdout, "  PIPE_CAPF_MAX_TEXTURE_LOD_BIAS\n");
		ret = 16.0f;
		break;

	default:
		fprintf(stdout, "  unsupported parameter: %d\n", param);
		ret = 0.0f;
		break;
	}

	//fprintf(stdout, "< %s() = %f\n", __func__, ret);
	return ret;
}

static int tegra_screen_get_shader_param(struct pipe_screen *pscreen,
					 unsigned int shader,
					 enum pipe_shader_cap param)
{
	int ret = 0;

	//fprintf(stdout, "> %s(pscreen=%p, shader=%u, param=%d)\n", __func__,
	//	pscreen, shader, param);

	switch (shader) {
	case PIPE_SHADER_VERTEX:
		fprintf(stdout, "  PIPE_SHADER_VERTEX: ");

		switch (param) {
		case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_INSTRUCTIONS\n");
			ret = 1024;
			break;

		case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS\n");
			ret = 1024;
			break;

		case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS\n");
			ret = 1024;
			break;

		case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS\n");
			ret = 8;
			break;

		case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH\n");
			ret = 4;
			break;

		case PIPE_SHADER_CAP_MAX_INPUTS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_INPUTS\n");
			ret = 16;
			break;

		case PIPE_SHADER_CAP_MAX_OUTPUTS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_OUTPUTS\n");
			ret = 16;
			break;

		case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE\n");
			ret = 64;
			break;

		case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_CONST_BUFFERS\n");
			ret = 64;
			break;

		case PIPE_SHADER_CAP_MAX_TEMPS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEMPS\n");
			ret = 16;
			break;

		case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
			fprintf(stdout, "PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_CONST_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_SUBROUTINES:
			fprintf(stdout, "PIPE_SHADER_CAP_SUBROUTINES\n");
			ret = 0;
			break;

		case PIPE_SHADER_CAP_INTEGERS:
			fprintf(stdout, "PIPE_SHADER_CAP_INTEGERS\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS\n");
			ret = 8;
			break;

		case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
			fprintf(stdout, "PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED\n");
			ret = 0;
			break;

		default:
			fprintf(stdout, "unsupported parameter: %d\n", param);
			ret = 0;
			break;
		}
		break;

	case PIPE_SHADER_FRAGMENT:
		fprintf(stdout, "  PIPE_SHADER_FRAGMENT: ");

		switch (param) {
		case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_INSTRUCTIONS\n");
			ret = 1024;
			break;

		case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS\n");
			ret = 1024;
			break;

		case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS\n");
			ret = 1024;
			break;

		case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS\n");
			ret = 8;
			break;

		case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH\n");
			ret = 4;
			break;

		case PIPE_SHADER_CAP_MAX_INPUTS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_INPUTS\n");
			ret = 16;
			break;

		case PIPE_SHADER_CAP_MAX_OUTPUTS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_OUTPUTS\n");
			ret = 16;
			break;

		case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE\n");
			ret = 64;
			break;

		case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_CONST_BUFFERS\n");
			ret = 64;
			break;

		case PIPE_SHADER_CAP_MAX_TEMPS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEMPS\n");
			ret = 16;
			break;

		case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
			fprintf(stdout, "PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
			fprintf(stdout, "PIPE_SHADER_CAP_INDIRECT_CONST_ADDR\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_SUBROUTINES:
			fprintf(stdout, "PIPE_SHADER_CAP_SUBROUTINES\n");
			ret = 0;
			break;

		case PIPE_SHADER_CAP_INTEGERS:
			fprintf(stdout, "PIPE_SHADER_CAP_INTEGERS\n");
			ret = 1;
			break;

		case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
			fprintf(stdout, "PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS\n");
			ret = 8;
			break;

		case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
			fprintf(stdout, "PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED\n");
			ret = 0;
			break;

		default:
			fprintf(stdout, "unsupported parameter: %d\n", param);
			ret = 0;
			break;
		}
		break;

	case PIPE_SHADER_GEOMETRY:
		fprintf(stdout, "  PIPE_SHADER_GEOMETRY not supported\n");
		ret = 0;
		break;

	case PIPE_SHADER_COMPUTE:
		fprintf(stdout, "  PIPE_SHADER_COMPUTE not supported\n");
		ret = 0;
		break;

	default:
		fprintf(stdout, "  unknown shader type: %u\n", shader);
		ret = 0;
		break;
	}

	//fprintf(stdout, "< %s() = %d\n", __func__, ret);
	return ret;
}

static boolean tegra_screen_is_format_supported(struct pipe_screen *pscreen,
						enum pipe_format format,
						enum pipe_texture_target target,
						unsigned int sample_count,
						unsigned int bindings)
{
	boolean ret = TRUE;
	fprintf(stdout, "> %s(pscreen=%p, format=%d, target=%d, sample_count=%u, bindings=%u)\n",
		__func__, pscreen, format, target, sample_count, bindings);
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

	screen->base.destroy = tegra_screen_destroy;
	screen->base.get_name = tegra_screen_get_name;
	screen->base.get_vendor = tegra_screen_get_vendor;
	screen->base.get_param = tegra_screen_get_param;
	screen->base.get_paramf = tegra_screen_get_paramf;
	screen->base.get_shader_param = tegra_screen_get_shader_param;
	screen->base.context_create = tegra_screen_context_create;
	screen->base.is_format_supported = tegra_screen_is_format_supported;

	tegra_screen_resource_init(&screen->base);

	fprintf(stdout, "< %s() = %p\n", __func__, &screen->base);
	return &screen->base;
}
