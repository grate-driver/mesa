#include "grate_compiler.h"
#include "grate_fp_ir.h"

#include "tgsi/tgsi_parse.h"

#include "util/u_memory.h"

static struct fp_alu_src_operand
fp_alu_src_row(int index)
{
   assert(index >= 0 && index < 16);
   struct fp_alu_src_operand src = {
      .index = index
   };
   return src;
}

static struct fp_alu_src_operand
fp_alu_src_reg(int index)
{
   assert(index >= 0 && index < 8);
   struct fp_alu_src_operand src = {
      .index = 16 + index
   };
   return src;
}

static struct fp_alu_src_operand
fp_alu_src_zero()
{
   struct fp_alu_src_operand src = {
      .index = 31,
      .datatype = FP_DATATYPE_FIXED10,
      .sub_reg_select_high = 0
   };
   return src;
}

static struct fp_alu_src_operand
fp_alu_src_one()
{
   struct fp_alu_src_operand src = {
      .index = 31,
      .datatype = FP_DATATYPE_FIXED10,
      .sub_reg_select_high = 1
   };
   return src;
}

static struct fp_alu_instr
fp_alu_sMOV(struct fp_alu_dst_operand dst, struct fp_alu_src_operand src)
{
   struct fp_alu_instr ret = {
      .op = FP_ALU_OP_MAD,
      .dst = dst,
      .src = {
         src,
         fp_alu_src_one(),
         fp_alu_src_zero(),
         fp_alu_src_one()
      }
   };
   return ret;
}

static struct fp_alu_dst_operand
fp_alu_dst(const struct tgsi_dst_register *dst, int subreg, bool saturate)
{
   struct fp_alu_dst_operand ret = { 0 };

   ret.index = dst->Index;
   if (dst->File == TGSI_FILE_OUTPUT) {
      ret.index = 2; // HACK: r2+r3 to match hard-coded store shader for now

      // fixed10
      // swizzle RGBA -> BGRA
      int o = subreg < 3 ? (2 - subreg) : 3;
      ret.index += o / 2;
      ret.write_low_sub_reg = (o % 2) == 0;
      ret.write_high_sub_reg = (o % 2) != 0;
   } else
      ret.index += subreg;

   ret.saturate = saturate;

   return ret;
}

static void
emit_vMOV(struct grate_fp_shader *fp, const struct tgsi_dst_register *dst,
          bool saturate, const struct tgsi_src_register *src)
{
   struct fp_instr *inst = CALLOC_STRUCT(fp_instr);
   list_inithead(&inst->link);

   struct fp_mfu_instr *mfu = NULL;
   if (src->File == TGSI_FILE_INPUT) {
      mfu = CALLOC_STRUCT(fp_mfu_instr);
      list_inithead(&mfu->link);
   }

   int swizzle[] = {
      src->SwizzleX,
      src->SwizzleY,
      src->SwizzleZ,
      src->SwizzleW
   };

   struct fp_alu_instr_packet *alu = CALLOC_STRUCT(fp_alu_instr_packet);
   int alu_instrs = 0;
   list_inithead(&alu->link);
   for (int i = 0; i < 4; ++i) {
      if ((dst->WriteMask & (1 << i)) == 0)
         continue;

      int comp = swizzle[i];

      struct fp_alu_src_operand src0 = { };
      if (src->File == TGSI_FILE_INPUT) {
         mfu->var[i].op = FP_VAR_OP_FP20;
         mfu->var[i].tram_row = src->Index;
         fp->info.max_tram_row = MAX2(fp->info.max_tram_row, src->Index);
         src0 = fp_alu_src_row(comp);
      } else
         src0 = fp_alu_src_reg(src->Index + comp);

      alu->slots[alu_instrs++] = fp_alu_sMOV(fp_alu_dst(dst, i, saturate), src0);
   }
   inst->alu_sched.num_instructions = 1;
   inst->alu_sched.address = list_length(&fp->fp_instructions);

   if (mfu != NULL) {
      inst->mfu_sched.num_instructions = 1;
      inst->mfu_sched.address = list_length(&fp->fp_instructions);
      list_addtail(&mfu->link, &fp->mfu_instructions);
   }

   if (dst->File == TGSI_FILE_OUTPUT) {
      inst->dw.enable = 1;
      inst->dw.index = 1 + dst->Index;
      inst->dw.stencil_write = 0;
      inst->dw.src_regs = FP_DW_REGS_R2_R3; // hard-coded for now
   }

   list_addtail(&alu->link, &fp->alu_instructions);
   list_addtail(&inst->link, &fp->fp_instructions);
}

static void
emit_tgsi_instr(struct grate_fp_shader *fp, const struct tgsi_full_instruction *inst)
{
   bool saturate = inst->Instruction.Saturate != 0;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
      emit_vMOV(fp, &inst->Dst[0].Register, saturate,
                    &inst->Src[0].Register);
      break;

   default:
      unreachable("unsupported TGSI-opcode!");
   }
}

#define LINK_SRC(index) ((index) << 3)
#define LINK_DST(index, comp, type) (((comp) | (type) << 2) << ((index) * 4))
#define LINK_DST_NONE      0
#define LINK_DST_FX10_LOW  1
#define LINK_DST_FX10_HIGH 2
#define LINK_DST_FP20      3

static void
emit_tgsi_input(struct grate_fp_shader *fp, const struct tgsi_full_declaration *decl)
{
   assert(decl->Range.First == decl->Range.Last);

   uint32_t src = LINK_SRC(1);
   uint32_t dst = 0;
   for (int i = 0; i < 4; ++i)
      dst |= LINK_DST(i, i, LINK_DST_FP20);

   fp->info.inputs[fp->info.num_inputs].src = src;
   fp->info.inputs[fp->info.num_inputs].dst = dst;

   if (decl->Declaration.Semantic == TGSI_SEMANTIC_COLOR)
      fp->info.color_input = decl->Range.First;

   fp->info.num_inputs++;
}

static void
emit_tgsi_declaration(struct grate_fp_shader *fp, const struct tgsi_full_declaration *decl)
{
   switch (decl->Declaration.File) {
   case TGSI_FILE_INPUT:
      emit_tgsi_input(fp, decl);
      break;
   }
}

void
grate_tgsi_to_fp(struct grate_fp_shader *fp, struct tgsi_parse_context *tgsi)
{
   list_inithead(&fp->fp_instructions);
   list_inithead(&fp->alu_instructions);
   list_inithead(&fp->mfu_instructions);

   fp->info.num_inputs = 0;
   fp->info.color_input = -1;
   fp->info.max_tram_row = 1;

   while (!tgsi_parse_end_of_tokens(tgsi)) {
      tgsi_parse_token(tgsi);
      switch (tgsi->FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         emit_tgsi_declaration(fp, &tgsi->FullToken.FullDeclaration);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (tgsi->FullToken.FullInstruction.Instruction.Opcode != TGSI_OPCODE_END)
            emit_tgsi_instr(fp, &tgsi->FullToken.FullInstruction);
         break;
      }
   }

   /*
    * HACK: insert barycentric interpolation setup
    * This will overwrite instructions in some cases, need proper scheduler
    * to fix properly
    */
    struct fp_mfu_instr *first = list_first_entry(&fp->mfu_instructions, struct fp_mfu_instr, link);
    first->sfu.op = FP_SFU_OP_RCP;
    first->sfu.reg = 4;
    first->mul[0].dst = FP_MFU_MUL_DST_BARYCENTRIC_WEIGHT;
    first->mul[0].src[0] = FP_MFU_MUL_SRC_SFU_RESULT;
    first->mul[0].src[1] = FP_MFU_MUL_SRC_BARYCENTRIC_COEF_0;

    first->mul[1].dst = FP_MFU_MUL_DST_BARYCENTRIC_WEIGHT;
    first->mul[1].src[0] = FP_MFU_MUL_SRC_SFU_RESULT;
    first->mul[1].src[1] = FP_MFU_MUL_SRC_BARYCENTRIC_COEF_1;
}
