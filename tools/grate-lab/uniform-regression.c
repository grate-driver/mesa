/* CPU-only: actual Gallium binding and fragment upload emitter with a mock stream. */
#include <stdio.h>
#include <assert.h>
#include "grate_context.h"
static uint32_t captured_uniforms[64];static unsigned num_captured;
static int test_stream_push(struct grate_stream *stream,uint32_t word){assert(num_captured<64);captured_uniforms[num_captured++]=word;return 0;}
#define grate_stream_push test_stream_push
#include "grate_state.c"
#undef grate_stream_push
static void check_uniform_upload(void){
 struct grate_context ctx={0};struct grate_channel channel={0};ctx.gr3d=&channel;
 float values[]={0.25f,0.75f,0.0f,1.0f},vertex[]={9,8,7,6};
 struct pipe_constant_buffer fragment={.user_buffer=values,.buffer_size=sizeof(values)};
 struct pipe_constant_buffer vert={.user_buffer=vertex,.buffer_size=sizeof(vertex)};
 grate_set_constant_buffer(&ctx.base,PIPE_SHADER_VERTEX,0,false,&vert);
 grate_set_constant_buffer(&ctx.base,PIPE_SHADER_FRAGMENT,0,false,&fragment);
 emit_fs_uniforms(&ctx);assert(num_captured==5);
 assert(captured_uniforms[0]==host1x_opcode_incr(TGR3D_FP_CONST(0),4));
 assert(captured_uniforms[1]==0x3a000 && captured_uniforms[2]==0x3d000 && captured_uniforms[3]==0 && captured_uniforms[4]==0x3e000);
 puts("PASS fragment PIPE-stage selection and exact FP20 upload (vertex sentinel isolated)");
}

int main(void){check_uniform_upload();return 0;}
