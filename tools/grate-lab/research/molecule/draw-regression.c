#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "grate_context.h"
#include "grate_resource.h"
#include "grate_program.h"
#include "grate_draw_limits.h"
#include "util/u_inlines.h"
#include "host1x01_hardware.h"
#include "tgr_3d.xml.h"
static uint32_t commands[2048];static unsigned used,submits,reloc_count,offsets[128];
static unsigned char index_data[1024*1024];static unsigned maps,unmaps,expected_user,expected_start,source_width;
static struct grate_resource uploaded;static unsigned allocation=1024*1024,upload_calls;
static int test_begin(struct grate_stream*s){used=0;return 0;}
static int test_end(struct grate_stream*s){return 0;}
static int test_flush(struct grate_stream*s);
static int test_push(struct grate_stream*s,uint32_t v){assert(used<2048);commands[used++]=v;return 0;}
static int test_class(struct grate_stream*s,uint32_t c){return 0;}
static int test_reloc(struct grate_stream*s,struct drm_tegra_bo*b,unsigned off){assert(reloc_count<128);offsets[reloc_count++]=off;return test_push(s,0xdeadbeef);}
static int test_size(struct drm_tegra_bo*b,uint32_t*out){*out=allocation;return 0;}
static void test_state(struct grate_context*c){}
static int test_map(struct drm_tegra_bo*b,void**ptr){maps++;*ptr=index_data;return 0;}
static int test_unmap(struct drm_tegra_bo*b){unmaps++;return 0;}
static bool test_upload(struct pipe_context*p,const struct pipe_draw_info*i,const struct pipe_draw_start_count_bias*d,struct pipe_resource**out,unsigned*off,unsigned align){
 upload_calls++;assert(align==64 && i->has_user_indices);
 assert(d->start==0);
 assert(i->index_size==(source_width==4?2:source_width));
 if(source_width==4){for(unsigned n=0;n<d->count;n++){uint32_t original;uint16_t actual;memcpy(&original,index_data+(expected_start+n)*4,4);memcpy(&actual,(const uint8_t*)i->index.user+n*2,2);assert(original==actual);}}
 else assert(memcmp(i->index.user,index_data+expected_start*source_width,d->count*source_width)==0);
 pipe_reference_init(&uploaded.b.reference,2);uploaded.b.width0=allocation;uploaded.bo=(void*)1;*out=&uploaded.b;
 unsigned first=d->start*i->index_size;*off=((first+63)&~63u)+64-first;return true;
}
#define drm_tegra_bo_map test_map
#define drm_tegra_bo_unmap test_unmap
#define grate_stream_begin test_begin
#define grate_stream_end test_end
#define grate_stream_flush test_flush
#define grate_stream_push test_push
#define grate_stream_push_setclass test_class
#define grate_stream_push_reloc test_reloc
#define drm_tegra_bo_get_size test_size
#define grate_emit_state test_state
#define util_upload_index_buffer test_upload
#include "grate_draw.c"
#undef grate_emit_state
static unsigned expected_first,expected_count,expected_index;
static int test_flush(struct grate_stream*s){
 unsigned params=0,prims=0;
 for(unsigned j=0;j+1<used;j++){
  if(commands[j]==host1x_opcode_incr(TGR3D_DRAW_PARAMS,1)){
   assert((commands[j+1]&TGR3D_DRAW_PARAMS_FIRST__MASK)==expected_first);
   assert(((commands[j+1]&TGR3D_DRAW_PARAMS_INDEX_MODE__MASK)>>TGR3D_DRAW_PARAMS_INDEX_MODE__SHIFT)==expected_index);params++;
  }
  if(commands[j]==host1x_opcode_incr(TGR3D_DRAW_PRIMITIVES,1)){
   unsigned take=expected_count>GRATE_DRAW_LIMIT?2046:expected_count;
   assert(((commands[j+1]&TGR3D_DRAW_PRIMITIVES_INDEX_COUNT__MASK)>>20)+1==take);
   unsigned size=expected_index==3?4:expected_index==2?2:1;
   assert((commands[j+1]&TGR3D_DRAW_PRIMITIVES_OFFSET__MASK)==(expected_index?0:expected_start));
   if(expected_index){unsigned pointer=64;assert(offsets[reloc_count-1]==pointer);}
   expected_start+=take;expected_count-=take;prims++;
  }
 }
 assert(params==1&&prims==1);submits++;return 0;
}
static unsigned long total_prims;
static unsigned prims(unsigned mode,unsigned count){switch(mode){case PIPE_PRIM_POINTS:return count;case PIPE_PRIM_LINES:return count/2;case PIPE_PRIM_LINE_STRIP:return count?count-1:0;case PIPE_PRIM_TRIANGLES:return count/3;case PIPE_PRIM_TRIANGLE_STRIP:return count>2?count-2:0;default:assert(0);return 0;}}
static unsigned tuple(unsigned mode,unsigned p,unsigned c){switch(mode){case PIPE_PRIM_POINTS:return p;case PIPE_PRIM_LINES:return p*2+c;case PIPE_PRIM_LINE_STRIP:return p+c;case PIPE_PRIM_TRIANGLES:return p*3+c;case PIPE_PRIM_TRIANGLE_STRIP:return p+((p&1)&&c<2?1-c:c);default:assert(0);return 0;}}
static void check_plan(void){
 unsigned modes[]={PIPE_PRIM_POINTS,PIPE_PRIM_LINES,PIPE_PRIM_LINE_STRIP,PIPE_PRIM_TRIANGLES,PIPE_PRIM_TRIANGLE_STRIP};unsigned counts[]={1,2,3,4,2045,2046,2047,2048,4095,4096,7050,8193,20000};unsigned starts[]={0,3,7806};
 for(unsigned m=0;m<5;m++)for(unsigned n=0;n<13;n++)for(unsigned z=0;z<3;z++){
  unsigned count=counts[n],mode=modes[m];if(!u_trim_pipe_prim(mode,&count))continue;unsigned remaining=count,start=starts[z],out=0;
  while(remaining){unsigned take,step;assert(grate_next_draw_chunk(mode,remaining,&take,&step));assert(take<=2047&&step>0&&step<=take&&start+take<=starts[z]+count);
   unsigned np=prims(mode,take),components=mode==PIPE_PRIM_POINTS?1:(mode==PIPE_PRIM_LINES||mode==PIPE_PRIM_LINE_STRIP)?2:3;
   for(unsigned p=0;p<np;p++){for(unsigned c=0;c<components;c++)assert(start+tuple(mode,p,c)==starts[z]+tuple(mode,out,c));out++;total_prims++;}
   start+=step;remaining-=step;
  }
  assert(out==prims(mode,count));
 }
 unsigned a,b;assert(!grate_next_draw_chunk(PIPE_PRIM_TRIANGLE_FAN,7050,&a,&b));assert(!grate_next_draw_chunk(PIPE_PRIM_LINE_LOOP,7050,&a,&b));assert(!grate_next_draw_chunk(PIPE_PRIM_POINTS,0,&a,&b));
 assert(grate_index_range_valid(7806,7050,4,59424,65536));assert(!grate_index_range_valid(7806,7050,4,59423,65536));assert(!grate_index_range_valid(7806,7050,4,65536,59423));assert(!grate_index_range_valid(UINT_MAX,1,4,UINT_MAX,UINT_MAX));assert(!grate_index_range_valid(0,3,3,64,64));
}
int main(void){
 uint32_t values[]={0,1,65535};uint8_t narrow[12];memcpy(narrow,values,12);assert(grate_index32_to_16(narrow,3));for(unsigned i=0;i<3;i++){uint16_t v;memcpy(&v,narrow+i*2,2);assert(v==values[i]);}values[0]=65536;memcpy(narrow,values,12);assert(!grate_index32_to_16(narrow,3));values[0]=UINT_MAX;memcpy(narrow,values,12);assert(!grate_index32_to_16(narrow,3));assert(!grate_index32_to_16(narrow,2048));
 check_plan();for(unsigned i=0;i<sizeof(index_data);i++)index_data[i]=(unsigned char)(i*31+7);
 struct grate_context ctx={0};struct grate_channel channel={0};struct grate_vertex_shader_state vs={0};struct grate_vertex_state vertex={0};struct grate_rasterizer_state rast={0};ctx.gr3d=&channel;ctx.vshader=&vs;ctx.vs=&vertex;ctx.rast=&rast;
 struct pipe_draw_info info={.mode=PIPE_PRIM_TRIANGLES,.instance_count=1};struct pipe_draw_start_count_bias draw={.start=7806,.count=7050};struct grate_resource indices={.b={.width0=1024*1024},.bo=(void*)1};pipe_reference_init(&indices.b.reference,1);
 grate_context_draw_init(&ctx.base);
 for(unsigned bytes=0;bytes<=4;bytes++){
  if(bytes==3)continue;
  for(unsigned user=0;user<(bytes?2:1);user++){
   for(unsigned n=0;bytes && n<sizeof(index_data)/bytes;n++) {uint32_t v=n%60000;if(bytes==4)memcpy(index_data+n*4,&v,4);else if(bytes==2){uint16_t w=v;memcpy(index_data+n*2,&w,2);}else index_data[n]=(uint8_t)v;}
   info.index_size=bytes;info.has_user_indices=user;if(user)info.index.user=index_data;else info.index.resource=&indices.b;
   expected_count=7050;expected_start=7806;expected_first=0;source_width=bytes;expected_index=bytes==4?2:bytes==2?2:bytes==1?1:0;expected_user=user;submits=reloc_count=upload_calls=maps=unmaps=0;
   ctx.base.draw_vbo(&ctx.base,&info,0,NULL,&draw,1);assert(submits==4&&expected_count==0);assert(draw.start==7806&&draw.count==7050);assert(indices.b.reference.count==1);if(bytes){assert(upload_calls==4&&uploaded.b.reference.count==1);assert(maps==(user?0:4)&&unmaps==maps);}
  }
 }
 struct grate_context empty={0};struct pipe_draw_info zero={.instance_count=1};struct pipe_draw_start_count_bias no={.count=0};grate_draw_vbo(&empty.base,&zero,0,NULL,NULL,0);grate_draw_vbo(&empty.base,&zero,0,NULL,&no,1);zero.instance_count=0;no.count=10;grate_draw_vbo(&empty.base,&zero,0,NULL,&no,1);
 printf("PASS draw planning: %lu independently compared primitives, strip winding/joins, 7050-index nonzero-start command capture for all index widths and user/resource ownership, empty draws and BO overflow guards\n",total_prims);return 0;
}
