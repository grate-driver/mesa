#include <stdio.h>
#include <string.h>

#include "util/u_dynarray.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_lowering.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_transform.h"

#include "host1x01_hardware.h"
#include "grate_common.h"
#include "grate_context.h"
#include "grate_screen.h"
#include "grate_program.h"
#include "grate_compiler.h"
#include "fp/fpir.h"
#include "vp/vpir.h"
#include "tgr_3d.xml.h"

struct grate_vs_tgsi_transform_context {
   struct tgsi_transform_context base;
   unsigned next_temp;
};

static void
grate_vs_tgsi_emit_native(struct tgsi_transform_context *ctx,
                                    struct tgsi_full_instruction *inst)
{
   struct grate_vs_tgsi_transform_context *gctx =
      (struct grate_vs_tgsi_transform_context *)ctx;

   int const_index = -1;
   int const_file = -1;
   for (unsigned i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      if (inst->Src[i].Register.File != TGSI_FILE_CONSTANT &&
          inst->Src[i].Register.File != TGSI_FILE_IMMEDIATE)
         continue;

      if (const_index < 0) {
         const_index = inst->Src[i].Register.Index;
         const_file = inst->Src[i].Register.File;
      } else if (inst->Src[i].Register.Index != const_index ||
                 inst->Src[i].Register.File != const_file) {
         /* We already have a constant-reads in this instruction,
          * add extra instructions in order to read more
          */
         unsigned temp = gctx->next_temp++;
         assert(temp < 64);
         tgsi_transform_temp_decl(ctx, temp);
         /* insert MOV to temp */
         tgsi_transform_op1_inst(ctx, TGSI_OPCODE_MOV,
                                 /* dst reg */
                                 TGSI_FILE_TEMPORARY, temp,
                                 TGSI_WRITEMASK_XYZW,
                                 /* src reg */
                                 inst->Src[i].Register.File,
                                 inst->Src[i].Register.Index);

         /* rewrite instruction */
	 tgsi_transform_src_reg_xyzw(&inst->Src[i], TGSI_FILE_TEMPORARY, temp);
	 inst->Src[i].Register.File = TGSI_FILE_TEMPORARY;
         inst->Src[i].Register.Index = temp;
         inst->Src[i].Register.Dimension = 0;
      }
   }

   ctx->emit_instruction(ctx, inst);
}

static void
grate_vs_tgsi_transform_instruction(struct tgsi_transform_context *ctx,
                                     struct tgsi_full_instruction *inst)
{
   if (inst->Instruction.Opcode != TGSI_OPCODE_CMP) {
      grate_vs_tgsi_emit_native(ctx, inst);
      return;
   }
   struct grate_vs_tgsi_transform_context *gctx =
      (struct grate_vs_tgsi_transform_context *)ctx;
   unsigned predicate = gctx->next_temp++, selected = gctx->next_temp++;
   assert(gctx->next_temp <= 64);
   tgsi_transform_temp_decl(ctx, predicate);
   tgsi_transform_temp_decl(ctx, selected);
   unsigned mask = inst->Dst[0].Register.WriteMask;
   struct tgsi_full_instruction op = tgsi_default_full_instruction();
   op.Instruction.NumDstRegs = 1;
   op.Instruction.NumSrcRegs = 2;
   op.Instruction.Opcode = TGSI_OPCODE_SLT;
   tgsi_transform_dst_reg(&op.Dst[0], TGSI_FILE_TEMPORARY, predicate, mask);
   op.Src[0] = inst->Src[0]; op.Src[1] = inst->Src[0];
   op.Src[1].Register.Negate = !op.Src[1].Register.Negate;
   grate_vs_tgsi_emit_native(ctx, &op);

   op = tgsi_default_full_instruction();
   op.Instruction.NumDstRegs = 1; op.Instruction.NumSrcRegs = 2;
   op.Instruction.Opcode = TGSI_OPCODE_MUL;
   tgsi_transform_dst_reg(&op.Dst[0], TGSI_FILE_TEMPORARY, selected, mask);
   tgsi_transform_src_reg_xyzw(&op.Src[0], TGSI_FILE_TEMPORARY, predicate);
   op.Src[1] = inst->Src[1];
   grate_vs_tgsi_emit_native(ctx, &op);

   op = tgsi_default_full_instruction();
   op.Instruction.NumDstRegs = 1; op.Instruction.NumSrcRegs = 3;
   op.Instruction.Opcode = TGSI_OPCODE_MAD;
   tgsi_transform_dst_reg(&op.Dst[0], TGSI_FILE_TEMPORARY, predicate, mask);
   tgsi_transform_src_reg_xyzw(&op.Src[0], TGSI_FILE_TEMPORARY, predicate);
   op.Src[0].Register.Negate = 1;
   op.Src[1] = inst->Src[2]; op.Src[2] = inst->Src[2];
   grate_vs_tgsi_emit_native(ctx, &op);

   op = tgsi_default_full_instruction();
   op.Instruction.NumDstRegs = 1; op.Instruction.NumSrcRegs = 2;
   op.Instruction.Opcode = TGSI_OPCODE_ADD;
   op.Instruction.Saturate = inst->Instruction.Saturate;
   op.Dst[0] = inst->Dst[0];
   tgsi_transform_src_reg_xyzw(&op.Src[0], TGSI_FILE_TEMPORARY, selected);
   tgsi_transform_src_reg_xyzw(&op.Src[1], TGSI_FILE_TEMPORARY, predicate);
   grate_vs_tgsi_emit_native(ctx, &op);
}

static struct tgsi_token *
grate_vs_tgsi_transform(const struct tgsi_token *tokens_in)
{
   const unsigned new_len = tgsi_num_tokens(tokens_in) * 8 + 256;

   struct tgsi_shader_info info;
   tgsi_scan_shader(tokens_in, &info);

   struct grate_vs_tgsi_transform_context ctx = {};
   ctx.base.transform_instruction = grate_vs_tgsi_transform_instruction;
   ctx.next_temp = info.file_max[TGSI_FILE_TEMPORARY] + 1;

   return tgsi_transform_shader(tokens_in, new_len, &ctx.base);
}


static void *
grate_create_vs_state(struct pipe_context *pcontext,
                      const struct pipe_shader_state *template)
{
   struct grate_vertex_shader_state *so =
      CALLOC_STRUCT(grate_vertex_shader_state);

   if (!so)
      return NULL;

   so->base = *template;

   if (grate_debug & GRATE_DEBUG_TGSI) {
      fprintf(stderr, "DEBUG: TGSI:\n");
      tgsi_dump(template->tokens, 0);
      fprintf(stderr, "\n");
   }

   struct tgsi_lowering_config lowering = { .lower_POW = 1 };
   struct tgsi_shader_info lowered_info;
   const struct tgsi_token *lowered = tgsi_transform_lowering(&lowering, template->tokens, &lowered_info);
   struct tgsi_token *new_tokens = grate_vs_tgsi_transform(lowered ? lowered : template->tokens);
   FREE((void *)lowered);
   if (!new_tokens)
      return NULL;

   struct tgsi_parse_context parser;
   unsigned ok = tgsi_parse_init(&parser, new_tokens);
   assert(ok == TGSI_PARSE_OK);

   struct grate_vp_shader vp;
   grate_tgsi_to_vp(&vp, &parser);

   int num_instructions = list_length(&vp.instructions);
   assert(num_instructions < 256);
   int num_commands = 2 + num_instructions * 4;
   uint32_t *commands = MALLOC(num_commands * sizeof(uint32_t));
   if (!commands) {
      FREE(so);
      return NULL;
   }

   commands[0] = host1x_opcode_imm(TGR3D_VP_UPLOAD_INST_ID, 0);
   commands[1] = host1x_opcode_nonincr(TGR3D_VP_UPLOAD_INST,
                                       num_instructions * 4);

   struct vp_instr *last = list_last_entry(&vp.instructions, struct vp_instr, link);
   int offset = 2;
   list_for_each_entry(struct vp_instr, instr, &vp.instructions, link) {
      bool end_of_program = instr == last;
      grate_vp_pack(commands + offset, instr, end_of_program);
      offset += 4;
   }

   so->blob.commands = commands;
   so->blob.num_commands = num_commands;
   so->output_mask = vp.output_mask;
   so->immediate_base = vp.immediate_base;
   so->num_immediates = vp.num_immediates;
   memcpy(so->immediates, vp.immediates, vp.num_immediates * sizeof(vp.immediates[0]));

   return so;
}

static void
grate_bind_vs_state(struct pipe_context *pcontext, void *so)
{
   grate_context(pcontext)->vshader = so;
}

static void
grate_delete_vs_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

/* Common source-over modes: premultiplied, straight, straight RGB/separate alpha. */
static void
append_source_over(struct grate_fp_shader *fp, unsigned mode)
{
   struct fp_instr *last = list_last_entry(&fp->fp_instructions, struct fp_instr, link);
   assert(last->dw.enable && last->dw.index == 1);
   unsigned source = last->dw.src_regs == FP_DW_REGS_R2_R3 ? 2 : 0;
   list_for_each_entry(struct fp_instr, i, &fp->fp_instructions, link)
      i->dw.enable = false;
   assert(list_length(&fp->fp_instructions) < 62);
   assert(list_length(&fp->alu_instructions) < 62);
   for (unsigned stage = 0; stage < 2; ++stage) {
      struct fp_instr *i = CALLOC_STRUCT(fp_instr);
      struct fp_alu_instr_packet *p = CALLOC_STRUCT(fp_alu_instr_packet);
      assert(i && p);
      i->alu_sched.address = list_length(&fp->alu_instructions);
      i->alu_sched.num_instructions = 1;
      for (unsigned c = 0; c < 4; ++c) {
         struct fp_alu_src_operand zero = { .index=31, .datatype=FP_DATATYPE_FIXED10 };
         struct fp_alu_src_operand one = zero; one.sub_reg_select_high = true;
         struct fp_alu_src_operand alpha = {
            .index=stage ? 3 : source+1, .datatype=FP_DATATYPE_FIXED10,
            .sub_reg_select_high=true
         };
         struct fp_alu_instr *alu = &p->slots[c];
         alu->op = FP_ALU_OP_MAD;
         alu->dst = (struct fp_alu_dst_operand) {
            .index=2+c/2, .write_low_sub_reg=!(c&1),
            .write_high_sub_reg=c&1, .saturate=true
         };
         alu->src[3] = one;
         if (!stage) {
            /* Preserve original alpha while premultiplying RGB. TEX may
             * leave its result in r0/r1: copy before framebuffer fetch. */
            alu->src[0] = (struct fp_alu_src_operand) {
               .index=source+c/2, .datatype=FP_DATATYPE_FIXED10,
               .sub_reg_select_high=c&1
            };
            alu->src[1] = mode != 1 && c < 3 ? alpha : one;
            alu->src[2] = zero;
         } else {
            alu->src[0] = (struct fp_alu_src_operand) {
               .index=c/2, .datatype=FP_DATATYPE_FIXED10,
               .sub_reg_select_high=c&1
            };
            alpha.minus_one=true; alpha.negate=true;
            alu->src[1] = alpha;
            alu->src[2] = (struct fp_alu_src_operand) {
               .index=2+c/2, .datatype=FP_DATATYPE_FIXED10,
               .sub_reg_select_high=c&1
            };
            if (mode == 2 && c == 3) {
               /* glBlendFunc(SRC_ALPHA,INV_SRC_ALPHA): alpha is a*a. */
               alu->src[3] = alu->src[2];
               alu->multiply_addend = true;
            }
         }
      }
      if (stage) {
         i->pseq_word=0x00810008; /* RT1 -> r0/r1 (OpenTegra framebuffer fetch). */
         i->dw=(struct fp_dw_instr){.enable=true,.index=1,.src_regs=FP_DW_REGS_R2_R3};
      }
      list_addtail(&p->link,&fp->alu_instructions);
      list_addtail(&i->link,&fp->fp_instructions);
   }
}

/* TXB takes S,T,R,bias from FP20 r0,r1,r2,r3. Interpolate/project
 * first, then load the bias: replacing r3 sooner would destroy TXP's q.
 * Uniform 31 is borrowed only in variants whose application never reads
 * CONST[7]. Zero-bias variants and their uniform space remain unchanged. */
static void
append_sampler_bias(struct grate_fp_shader *fp)
{
   list_for_each_entry_safe(struct fp_instr, tex, &fp->fp_instructions, link) {
      if (!tex->tex_word) continue;
      assert(list_length(&fp->fp_instructions) < 62);
      assert(list_length(&fp->alu_instructions) < 64);
      struct fp_instr *coord=CALLOC_STRUCT(fp_instr);
      struct fp_instr *bias=CALLOC_STRUCT(fp_instr);
      struct fp_alu_instr_packet *packet=CALLOC_STRUCT(fp_alu_instr_packet);
      assert(coord && bias && packet);
      coord->mfu_sched=tex->mfu_sched;
      memset(&tex->mfu_sched,0,sizeof(tex->mfu_sched));
      bias->alu_sched.address=list_length(&fp->alu_instructions);
      bias->alu_sched.num_instructions=1;
      for(unsigned i=0;i<4;i++) {
         packet->slots[i]=(struct fp_alu_instr) {
            .op=FP_ALU_OP_MAD, .dst={.index=31},
            .src={
               {.index=31,.datatype=FP_DATATYPE_FIXED10},
               {.index=31,.datatype=FP_DATATYPE_FIXED10,.sub_reg_select_high=true},
               {.index=31,.datatype=FP_DATATYPE_FIXED10},
               {.index=31,.datatype=FP_DATATYPE_FIXED10,.sub_reg_select_high=true}
            }
         };
      }
      /* Both write enables are required for a full FP20 result (.lh).
       * A zero mask discards the ALU result, it does not mean FP20. */
      packet->slots[0].dst.index=3;
      packet->slots[0].dst.write_low_sub_reg=true;
      packet->slots[0].dst.write_high_sub_reg=true;
      packet->slots[0].src[0]=(struct fp_alu_src_operand){.index=63};
      list_addtail(&packet->link,&fp->alu_instructions);
      list_addtail(&coord->link,&tex->link);
      list_addtail(&bias->link,&tex->link);
      tex->tex_word |= 1u << 12;
   }
}

static void
inspect_sampler_bias(struct grate_fragment_shader_state *so,
                     const struct tgsi_token *tokens)
{
   struct tgsi_parse_context p;
   assert(tgsi_parse_init(&p,tokens)==TGSI_PARSE_OK);
   while (!tgsi_parse_end_of_tokens(&p)) {
      tgsi_parse_token(&p);
      if (p.FullToken.Token.Type != TGSI_TOKEN_TYPE_INSTRUCTION) continue;
      const struct tgsi_full_instruction *i=&p.FullToken.FullInstruction;
      if (i->Instruction.Opcode==TGSI_OPCODE_TEX || i->Instruction.Opcode==TGSI_OPCODE_TXP)
         so->uses_texture=true;
      for(unsigned k=0;k<i->Instruction.NumSrcRegs;k++)
         if(i->Src[k].Register.File==TGSI_FILE_CONSTANT &&
            (i->Src[k].Register.Index>=7 || i->Src[k].Register.Indirect))
            so->bias_uniform_conflict=true;
   }
   tgsi_parse_free(&p);
}

static bool
grate_create_fs_blob(struct grate_context *context, const struct tgsi_token *tokens,
                     unsigned blend_mode, bool sampler_bias, struct grate_shader_blob *blob,
                     struct grate_fp_info *info)
{
   struct tgsi_parse_context parser;
   unsigned ok = tgsi_parse_init(&parser, tokens);
   assert(ok == TGSI_PARSE_OK);

   struct grate_fp_shader fp;
   grate_tgsi_to_fp(&fp, &parser);
   if (sampler_bias) append_sampler_bias(&fp);
   if (blend_mode) append_source_over(&fp, blend_mode);

   struct util_dynarray buf;
   util_dynarray_init(&buf, NULL);

#define PUSH(x) util_dynarray_append(&buf, uint32_t, (x))
   int num_fp_instrs = list_length(&fp.fp_instructions);
   assert(num_fp_instrs < 64);

   PUSH(host1x_opcode_incr(TGR3D_ALU_BUFFER_SIZE, 1));
   PUSH(num_fp_instrs > 1 ? 0x4a000000 : 0x58000000);
   if (num_fp_instrs > 1) {
      /* OpenTegra's matching one-row ALU buffer / recirculation setup. */
      PUSH(host1x_opcode_incr(0x501, 1));
      PUSH(0x223212cf);
   }

   PUSH(host1x_opcode_imm(TGR3D_FP_PSEQ_QUAD_ID, 0));
   PUSH(host1x_opcode_imm(TGR3D_FP_UPLOAD_INST_ID_COMMON, 0));
   PUSH(host1x_opcode_imm(TGR3D_FP_UPLOAD_MFU_INST_ID, 0));
   PUSH(host1x_opcode_imm(TGR3D_FP_UPLOAD_ALU_INST_ID, 0));


   PUSH(host1x_opcode_incr(TGR3D_FP_PSEQ_ENGINE_INST, 1));
   PUSH(0x20006000 | num_fp_instrs);

   unsigned exec = 0, dw_exec = 0;
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link) {
      exec++;
      if (instr->dw.enable)
         dw_exec = exec;
   }
   assert(dw_exec && dw_exec <= (unsigned)num_fp_instrs);
   PUSH(host1x_opcode_incr(TGR3D_FP_PSEQ_DW_CFG, 1));
   PUSH(TGR3D_VAL(FP_PSEQ_DW_CFG, PSEQ_TO_DW_EXEC_NB, dw_exec));

   if (context->tegra114) {
      /* XXX: maybe not needed */
      PUSH(host1x_opcode_incr(0x547, 0x0002));
      PUSH(0xc0000000);
      PUSH(0x00000000);
   }


   PUSH(host1x_opcode_nonincr(TGR3D_FP_PSEQ_UPLOAD_INST, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(instr->pseq_word);
   /* OpenTegra gen_shader_bin.c commits the PSEQ buffer AFTER upload. */
   PUSH(host1x_opcode_imm(TGR3D_FP_PSEQ_UPLOAD_INST_BUFFER_FLUSH, 0));

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_MFU_SCHED, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(grate_fp_pack_sched(&instr->mfu_sched));

   int num_mfu_instrs = list_length(&fp.mfu_instructions);
   assert(num_mfu_instrs < 64); // TODO: not sure if this is really correct

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_MFU_INST, num_mfu_instrs * 2));
   list_for_each_entry(struct fp_mfu_instr, instr, &fp.mfu_instructions, link) {
      uint32_t words[2];
      grate_fp_pack_mfu(words, instr);
      PUSH(words[0]);
      PUSH(words[1]);
   }

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_TEX_INST, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link) {
      PUSH(instr->tex_word);
   }

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_SCHED, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link) {
      if (context->tegra114)
         PUSH(grate_fp_pack_alu_sched_t114(&instr->alu_sched));
      else
         PUSH(grate_fp_pack_sched(&instr->alu_sched));
   }

   int num_alu_instrs = list_length(&fp.alu_instructions);
   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_INST,
        num_alu_instrs * 4 * 2));
   list_for_each_entry(struct fp_alu_instr_packet, instr, &fp.alu_instructions, link) {
      for (int i = 0; i < 4; ++i) {
         uint32_t words[2];
         grate_fp_pack_alu(words, instr->slots + i);
         PUSH(words[0]);
         PUSH(words[1]);
      }
   }

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_INST_COMPLEMENT, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(0x00000000);

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_DW_INST, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(grate_fp_pack_dw(&instr->dw));

   uint32_t tram_setup = 0;
   tram_setup |= TGR3D_VAL(TRAM_SETUP, USED_TRAM_ROWS_NB, fp.info.max_tram_row);
   tram_setup |= TGR3D_VAL(TRAM_SETUP, DIV64, 64 / fp.info.max_tram_row);

   PUSH(host1x_opcode_incr(TGR3D_TRAM_SETUP, 1));
   PUSH(tram_setup);

#undef PUSH
   util_dynarray_trim(&buf);

   blob->num_commands = buf.size / sizeof(uint32_t);
   blob->commands = buf.data;
   *info = fp.info;
   tgsi_parse_free(&parser);
   list_for_each_entry_safe(struct fp_instr, i, &fp.fp_instructions, link) { list_del(&i->link); FREE(i); }
   list_for_each_entry_safe(struct fp_alu_instr_packet, i, &fp.alu_instructions, link) { list_del(&i->link); FREE(i); }
   list_for_each_entry_safe(struct fp_mfu_instr, i, &fp.mfu_instructions, link) { list_del(&i->link); FREE(i); }
   return true;
}

static void *
grate_create_fs_state(struct pipe_context *pcontext, const struct pipe_shader_state *template)
{
   struct grate_fragment_shader_state *so = CALLOC_STRUCT(grate_fragment_shader_state);
   if (!so) return NULL;
   so->base=*template;
   if (grate_debug & GRATE_DEBUG_TGSI) tgsi_dump(template->tokens,0);
   struct grate_context *ctx=grate_context(pcontext);
   grate_create_fs_blob(ctx,template->tokens,0,false,&so->blob,&so->info);
   for (unsigned mode=1;mode<=3;++mode)
      grate_create_fs_blob(ctx,template->tokens,mode,false,&so->blend_blob[mode-1],&so->info);
   inspect_sampler_bias(so,template->tokens);
   if(so->uses_texture && !so->bias_uniform_conflict)
      for(unsigned mode=0;mode<4;mode++)
         grate_create_fs_blob(ctx,template->tokens,mode,true,&so->biased_blob[mode],&so->info);
   return so;
}

static void
grate_bind_fs_state(struct pipe_context *pcontext, void *so)
{
   grate_context(pcontext)->fshader = so;
}

static void
grate_delete_fs_state(struct pipe_context *pcontext, void *so)
{
   struct grate_fragment_shader_state *s=so;
   FREE(s->blob.commands);
   for(unsigned i=0;i<3;++i) FREE(s->blend_blob[i].commands);
   for(unsigned i=0;i<4;++i) FREE(s->biased_blob[i].commands);
   FREE(s);
}

void
grate_context_program_init(struct pipe_context *pcontext)
{
   pcontext->create_vs_state = grate_create_vs_state;
   pcontext->bind_vs_state = grate_bind_vs_state;
   pcontext->delete_vs_state = grate_delete_vs_state;

   pcontext->create_fs_state = grate_create_fs_state;
   pcontext->bind_fs_state = grate_bind_fs_state;
   pcontext->delete_fs_state = grate_delete_fs_state;
}
