#include "grate_context.h"
static uint32_t words[16];static unsigned count,allocation;
static int test_push(struct grate_stream*s,uint32_t word){assert(count<16);words[count++]=word;return 0;}
static int test_reloc(struct grate_stream*s,struct drm_tegra_bo*bo,unsigned offset){assert(offset==0);return test_push(s,0xdeadbeef);}
static int test_size(struct drm_tegra_bo*bo,uint32_t*size){*size=allocation;return 0;}
#define grate_stream_push test_push
#define grate_stream_push_reloc test_reloc
#define drm_tegra_bo_get_size test_size
#include "grate_state.c"
int main(void){
 struct grate_context ctx={0};struct grate_channel channel={0};ctx.gr3d=&channel;
 struct grate_resource res={.b={.target=PIPE_TEXTURE_2D,.width0=128,.height0=128,.depth0=1,.array_size=1,.last_level=7,.format=PIPE_FORMAT_B8G8R8A8_UNORM},.format=TGR3D_PIXEL_FORMAT_RGBA8888,.pitch=512,.bo=(void*)1};
 assert(grate_mip_layout_init(&res.mip,128,128,4,8,16));allocation=res.mip.total;assert(allocation==87408);
 struct pipe_sampler_view view={.texture=&res.b,.format=res.b.format};view.u.tex.first_level=0;view.u.tex.last_level=7;
 struct pipe_sampler_state sampler={.min_mip_filter=PIPE_TEX_MIPFILTER_NEAREST,.min_img_filter=PIPE_TEX_FILTER_NEAREST,.mag_img_filter=PIPE_TEX_FILTER_NEAREST,.min_lod=0,.max_lod=7,.wrap_s=PIPE_TEX_WRAP_CLAMP_TO_EDGE,.wrap_t=PIPE_TEX_WRAP_CLAMP_TO_EDGE};
 ctx.fragment_sampler_view=&view;ctx.fragment_sampler=&sampler;emit_simple_texture(&ctx);
 assert(count==5 && words[0]==host1x_opcode_incr(TGR3D_TEXTURE_POINTER(0),1) && words[1]==0xdeadbeef && words[2]==host1x_opcode_incr(TGR3D_TEXTURE_DESC1(0),2));
 assert((words[4]&TGR3D_TEXTURE_DESC2_MAX_LOD__MASK)==TGR3D_VAL(TEXTURE_DESC2,MAX_LOD,7));assert(!(words[4]&TGR3D_TEXTURE_DESC2_MIPMAP_DISABLE));
 count=0;sampler.min_mip_filter=PIPE_TEX_MIPFILTER_NONE;emit_simple_texture(&ctx);assert(count==5 && (words[4]&TGR3D_TEXTURE_DESC2_MIPMAP_DISABLE));
 puts("PASS actual sampler emitter: complete 16-byte chain, GPU maxLOD7, level-zero regression");return 0;
}
