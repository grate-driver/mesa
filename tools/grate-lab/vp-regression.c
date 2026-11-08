/* CPU-only regression: actual TGSI lowering + GRATE VP IR versus independent
 * interpretation of the original shader. No context, DRM, X11, or GPU calls. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tgsi/tgsi_text.h"
#include "grate_program.c"
static float uniforms[256][4],attribs[16][4],original[16][4],compiled[16][4];
static unsigned rng=0x713589u;
static float random_float(void){rng=rng*1664525u+1013904223u;return (float)(rng&65535u)/32768.0f-1;}
static void put(float dst[4],unsigned mask,const float v[4],int sat){for(int c=0;c<4;c++)if(mask&(1u<<c))dst[c]=sat?fminf(1,fmaxf(0,v[c])):v[c];}
static void tgsi_src(float v[4],const struct tgsi_src_register*s,float tmp[64][4],float imm[64][4]){
 const float*p=NULL;unsigned sw[4]={s->SwizzleX,s->SwizzleY,s->SwizzleZ,s->SwizzleW};
 switch(s->File){case TGSI_FILE_CONSTANT:p=uniforms[s->Index];break;case TGSI_FILE_TEMPORARY:p=tmp[s->Index];break;case TGSI_FILE_IMMEDIATE:p=imm[s->Index];break;case TGSI_FILE_INPUT:p=attribs[s->Index];break;default:abort();}
 for(int c=0;c<4;c++){v[c]=p[sw[c]];if(s->Absolute)v[c]=fabsf(v[c]);if(s->Negate)v[c]=-v[c];}
}
static void reference(const struct tgsi_token*tokens){
 struct tgsi_parse_context p;assert(tgsi_parse_init(&p,tokens)==TGSI_PARSE_OK);
 float tmp[64][4]={{0}},imm[64][4]={{0}};unsigned ni=0;memset(original,0,sizeof(original));
 while(!tgsi_parse_end_of_tokens(&p)){
  tgsi_parse_token(&p);
  if(p.FullToken.Token.Type==TGSI_TOKEN_TYPE_IMMEDIATE){const struct tgsi_full_immediate*i=&p.FullToken.FullImmediate;for(unsigned c=0;c<i->Immediate.NrTokens-1;c++)imm[ni][c]=i->u[c].Float;ni++;continue;}
  if(p.FullToken.Token.Type!=TGSI_TOKEN_TYPE_INSTRUCTION)continue;
  const struct tgsi_full_instruction*i=&p.FullToken.FullInstruction;if(i->Instruction.Opcode==TGSI_OPCODE_END)break;
  float s[3][4]={{0}},v[4]={0};for(unsigned k=0;k<i->Instruction.NumSrcRegs;k++)tgsi_src(s[k],&i->Src[k].Register,tmp,imm);
  for(int c=0;c<4;c++)switch(i->Instruction.Opcode){
   case TGSI_OPCODE_MOV:v[c]=s[0][c];break;
   case TGSI_OPCODE_ADD:v[c]=s[0][c]+s[1][c];break;
   case TGSI_OPCODE_MUL:v[c]=s[0][c]*s[1][c];break;
   case TGSI_OPCODE_MAD:v[c]=fmaf(s[0][c],s[1][c],s[2][c]);break;
   case TGSI_OPCODE_DP3:v[c]=s[0][0]*s[1][0]+s[0][1]*s[1][1]+s[0][2]*s[1][2];break;
   case TGSI_OPCODE_DP4:v[c]=s[0][0]*s[1][0]+s[0][1]*s[1][1]+s[0][2]*s[1][2]+s[0][3]*s[1][3];break;
   case TGSI_OPCODE_RSQ:v[c]=1/sqrtf(s[0][0]);break;
   case TGSI_OPCODE_MIN:v[c]=fminf(s[0][c],s[1][c]);break;
   case TGSI_OPCODE_MAX:v[c]=fmaxf(s[0][c],s[1][c]);break;
   case TGSI_OPCODE_SLT:v[c]=s[0][c]<s[1][c];break;
   case TGSI_OPCODE_SGE:v[c]=s[0][c]>=s[1][c];break;
   case TGSI_OPCODE_CMP:v[c]=s[0][c]<0?s[1][c]:s[2][c];break;
   case TGSI_OPCODE_POW:v[c]=powf(s[0][0],s[1][0]);break;
   default:fprintf(stderr,"unknown reference opcode %u\n",i->Instruction.Opcode);abort();
  }
  const struct tgsi_dst_register*d=&i->Dst[0].Register;float*dst=d->File==TGSI_FILE_OUTPUT?original[d->Index]:tmp[d->Index];put(dst,d->WriteMask,v,i->Instruction.Saturate);
 }
 tgsi_parse_free(&p);
}
static void vp_src(float v[4],struct vp_src_operand s,float tmp[64][4],float constants[256][4]){
 const float*p=NULL;
 switch(s.file){case VP_SRC_FILE_UNIFORM:p=constants[s.index];break;case VP_SRC_FILE_TEMP:p=tmp[s.index];break;case VP_SRC_FILE_ATTRIB:p=attribs[s.index];break;case VP_SRC_FILE_UNDEF:memset(v,0,16);return;}
 assert(p);
 for(int c=0;c<4;c++){v[c]=p[s.swizzle[c]];if(s.absolute)v[c]=fabsf(v[c]);if(s.negate)v[c]=-v[c];}
}
static void simulate(struct grate_vp_shader*vp){
 float tmp[64][4]={{0}},constants[256][4];memcpy(constants,uniforms,sizeof(constants));memset(compiled,0,sizeof(compiled));
 memcpy(constants+vp->immediate_base,vp->immediates,vp->num_immediates*16);
 list_for_each_entry(struct vp_instr,i,&vp->instructions,link){
  if(i->vec.op!=VP_VEC_OP_NOP){
   float s[3][4],v[4];for(int k=0;k<3;k++)vp_src(s[k],i->vec.src[k],tmp,constants);
   for(int c=0;c<4;c++)switch(i->vec.op){
    case VP_VEC_OP_MOV:v[c]=s[0][c];break;case VP_VEC_OP_ADD:v[c]=s[0][c]+s[2][c];break;
    case VP_VEC_OP_MUL:v[c]=s[0][c]*s[1][c];break;case VP_VEC_OP_MAD:v[c]=fmaf(s[0][c],s[1][c],s[2][c]);break;
    case VP_VEC_OP_DP3:v[c]=s[0][0]*s[1][0]+s[0][1]*s[1][1]+s[0][2]*s[1][2];break;
    case VP_VEC_OP_DP4:v[c]=s[0][0]*s[1][0]+s[0][1]*s[1][1]+s[0][2]*s[1][2]+s[0][3]*s[1][3];break;
    case VP_VEC_OP_MIN:v[c]=fminf(s[0][c],s[1][c]);break;case VP_VEC_OP_MAX:v[c]=fmaxf(s[0][c],s[1][c]);break;
    case VP_VEC_OP_SLT:v[c]=s[0][c]<s[1][c];break;case VP_VEC_OP_SGE:v[c]=s[0][c]>=s[1][c];break;
    default:abort();
   }
   struct vp_dst_operand d=i->vec.dst;put(d.file==VP_DST_FILE_OUTPUT?compiled[d.index]:tmp[d.index],d.write_mask,v,d.saturate);
  }
  if(i->scalar.op!=VP_SCALAR_OP_NOP){
   float s[4],v[4],r;vp_src(s,i->scalar.src,tmp,constants);
   switch(i->scalar.op){case VP_SCALAR_OP_RSQ:r=1/sqrtf(s[0]);break;case VP_SCALAR_OP_RCP:r=1/s[0];break;case VP_SCALAR_OP_LG2:r=log2f(s[0]);break;case VP_SCALAR_OP_EX2:r=exp2f(s[0]);break;default:abort();}
   for(int c=0;c<4;c++)v[c]=r;
   struct vp_dst_operand d=i->scalar.dst;
   put(d.file==VP_DST_FILE_OUTPUT?compiled[d.index]:tmp[d.index],d.write_mask,v,d.saturate);
  }
 }
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);char text[65536]={0};assert(fread(text,1,sizeof(text)-1,f)>0);fclose(f);
 struct tgsi_token*tokens=calloc(65536,sizeof(*tokens));assert(tokens&&tgsi_text_translate(text,tokens,65536));
 struct tgsi_lowering_config cfg={.lower_POW=1};struct tgsi_shader_info info;
 const struct tgsi_token*lowered=tgsi_transform_lowering(&cfg,tokens,&info);
 struct tgsi_token*transformed=grate_vs_tgsi_transform(lowered?lowered:tokens);assert(transformed);
 struct tgsi_parse_context parser;assert(tgsi_parse_init(&parser,transformed)==TGSI_PARSE_OK);
 struct grate_vp_shader vp;grate_tgsi_to_vp(&vp,&parser);tgsi_parse_free(&parser);
 assert(vp.immediate_base+vp.num_immediates<=256);
 for(int trial=0;trial<128;trial++){
  for(int r=0;r<256;r++)for(int c=0;c<4;c++)uniforms[r][c]=random_float();
  for(int r=0;r<16;r++)for(int c=0;c<4;c++)attribs[r][c]=random_float();
  uniforms[0][0]=40;
  reference(tokens);simulate(&vp);
  for(int r=0;r<16;r++)if(vp.output_mask&(1u<<r))for(int c=0;c<4;c++){
   float a=original[r][c],b=compiled[r][c];
   if(!(isfinite(a)&&isfinite(b)&&fabsf(a-b)<=.001f*(1+fabsf(a)))){
    fprintf(stderr,"mismatch trial=%d output=%d.%d reference=%g compiled=%g\n",trial,r,c,a,b);return 10;
   }
  }
 }
 /* Pack all instructions as well: checks single attribute/uniform read ports. */
 list_for_each_entry(struct vp_instr,i,&vp.instructions,link){uint32_t words[4];grate_vp_pack(words,i,false);}
 printf("{\"passed\":true,\"randomized_trials\":128,\"instructions\":%u,\"immediate_base\":%u,\"immediates\":%u}\n",list_length(&vp.instructions),vp.immediate_base,vp.num_immediates);
 return 0;
}
