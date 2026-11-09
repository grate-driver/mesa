#include "grate_compiler.h"
#include "fpir.h"

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
   } else {
      ret.index += subreg;
      /* TEMP coordinates are full FP20: enable both halves, like rN.lh. */
      ret.write_low_sub_reg = true;
      ret.write_high_sub_reg = true;
   }

   ret.saturate = saturate;

   return ret;
}

static void
emit_vMOV(struct grate_fp_shader *fp, const struct tgsi_dst_register *dst,
          bool saturate, const struct tgsi_src_register *src)
{
   if (dst->File == TGSI_FILE_TEMPORARY) {
      /* The restricted coordinate path stores vec4 TEMP[0] in row r0-r3. */
      assert(dst->Index == 0 && !saturate && !src->Negate && !src->Absolute &&
             (src->File == TGSI_FILE_INPUT || src->File == TGSI_FILE_CONSTANT));
      fp->simple_coord_mask |= dst->WriteMask;
      if (src->File == TGSI_FILE_CONSTANT)
         fp->simple_uniform_coord_mask |= dst->WriteMask;
      else
         fp->simple_uniform_coord_mask &= ~dst->WriteMask;
   }
   struct fp_instr *inst = CALLOC_STRUCT(fp_instr);
   list_inithead(&inst->link);

   struct fp_mfu_instr *mfu = NULL;
   if (src->File == TGSI_FILE_INPUT ||
       list_is_empty(&fp->mfu_instructions)) {
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

   /* Unused ALU slots must write zero to the discard register. */
   struct fp_alu_dst_operand nop_dst = { .index = 31 };
   for (int i = 0; i < 4; ++i)
      alu->slots[i] = fp_alu_sMOV(nop_dst, fp_alu_src_zero());

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
      } else if (src->File == TGSI_FILE_CONSTANT) {
         assert(src->Index * 4 + comp < 32);
         src0.index = 32 + src->Index * 4 + comp;
      } else {
         src0 = fp_alu_src_reg(src->Index + comp);
      }

      alu->slots[alu_instrs++] = fp_alu_sMOV(fp_alu_dst(dst, i, saturate), src0);
   }
   inst->alu_sched.num_instructions = 1;
   inst->alu_sched.address = list_length(&fp->alu_instructions);

   if (mfu != NULL) {
      inst->mfu_sched.num_instructions = 1;
      inst->mfu_sched.address = list_length(&fp->mfu_instructions);
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
emit_simple_tex(struct grate_fp_shader *fp,
                const struct tgsi_full_instruction *inst)
{
   const struct tgsi_dst_register *dst = &inst->Dst[0].Register;
   const struct tgsi_src_register *coord = &inst->Src[0].Register;
   const struct tgsi_src_register *sampler = &inst->Src[1].Register;

   assert((dst->File == TGSI_FILE_OUTPUT ||
           dst->File == TGSI_FILE_TEMPORARY) &&
          dst->Index == 0 && dst->WriteMask == 0xf);
   assert(fp->simple_tex_temp == -1);
   bool project = inst->Instruction.Opcode == TGSI_OPCODE_TXP;
   assert(!inst->Instruction.Saturate && !coord->Negate && !coord->Absolute);
   assert(coord->Index == 0 && coord->SwizzleX == TGSI_SWIZZLE_X &&
          coord->SwizzleY == TGSI_SWIZZLE_Y);
   assert(inst->Texture.Texture == TGSI_TEXTURE_2D);
   unsigned required = 3 | (project ? 1u << coord->SwizzleW : 0);
   assert(coord->File == TGSI_FILE_INPUT ||
          (coord->File == TGSI_FILE_TEMPORARY &&
           (fp->simple_coord_mask & required) == required));
   assert(sampler->File == TGSI_FILE_SAMPLER &&
          sampler->Index == 0);

   struct fp_instr *out = CALLOC_STRUCT(fp_instr);
   struct fp_mfu_instr *mfu = CALLOC_STRUCT(fp_mfu_instr);
   assert(out && mfu);
   list_inithead(&out->link);
   list_inithead(&mfu->link);

   if (coord->File == TGSI_FILE_INPUT) {
      for (unsigned i = 0; i < 4; i++) {
         if (required & (1u << i)) {
            mfu->var[i].op = FP_VAR_OP_FP20;
            mfu->var[i].tram_row = coord->Index;
         }
      }
   }
   out->mfu_sched.num_instructions = 1;
   out->mfu_sched.address = list_length(&fp->mfu_instructions);
   list_addtail(&mfu->link, &fp->mfu_instructions);

   if (project) {
      struct fp_mfu_instr *divide = CALLOC_STRUCT(fp_mfu_instr);
      assert(divide);
      list_inithead(&divide->link);
      divide->sfu.op = FP_SFU_OP_RCP;
      divide->sfu.reg = coord->SwizzleW;
      divide->mul[0].dst = FP_MFU_MUL_DST_ROW_REG_0;
      divide->mul[0].src[0] = FP_MFU_MUL_SRC_ROW_REG_0;
      divide->mul[0].src[1] = FP_MFU_MUL_SRC_SFU_RESULT;
      divide->mul[1].dst = FP_MFU_MUL_DST_ROW_REG_1;
      divide->mul[1].src[0] = FP_MFU_MUL_SRC_ROW_REG_1;
      divide->mul[1].src[1] = FP_MFU_MUL_SRC_SFU_RESULT;
      out->mfu_sched.num_instructions++;
      list_addtail(&divide->link, &fp->mfu_instructions);
   }
   fp->simple_coord_mask = 0; /* TEX overwrites the coordinate rows. */
   if (coord->File == TGSI_FILE_TEMPORARY &&
       (fp->simple_uniform_coord_mask & required))
      fp->info.unverified_uniform_texcoords = true;
   fp->simple_uniform_coord_mask = 0;
   out->tex_word = 0x00000400; /* TEX enable, sampler 0, r0/r1 */
   out->dw.enable = dst->File == TGSI_FILE_OUTPUT;
   out->dw.index = 1;
   out->dw.src_regs = FP_DW_REGS_R0_R1;
   if (dst->File == TGSI_FILE_TEMPORARY)
      fp->simple_tex_temp = dst->Index;
   list_addtail(&out->link, &fp->fp_instructions);
}


static void
emit_simple_tex_mul(struct grate_fp_shader *fp,
                    const struct tgsi_full_instruction *inst)
{
   const struct tgsi_dst_register *dst = &inst->Dst[0].Register;
   const struct tgsi_src_register *tex = &inst->Src[0].Register;
   const struct tgsi_src_register *factor = &inst->Src[1].Register;

   /* Initial implementation: texture result times a uniform. */
   assert(fp->simple_tex_temp == 0);
   assert(!list_is_empty(&fp->fp_instructions));

   assert(dst->File == TGSI_FILE_OUTPUT &&
          dst->Index == 0 && dst->WriteMask == 0xf);
   assert(tex->File == TGSI_FILE_TEMPORARY && tex->Index == 0 &&
          tex->SwizzleX == TGSI_SWIZZLE_X &&
          tex->SwizzleY == TGSI_SWIZZLE_Y &&
          tex->SwizzleZ == TGSI_SWIZZLE_Z &&
          tex->SwizzleW == TGSI_SWIZZLE_W &&
          !tex->Negate && !tex->Absolute);
   assert(factor->File == TGSI_FILE_CONSTANT &&
          !factor->Negate && !factor->Absolute);

   struct fp_instr *batch =
      list_last_entry(&fp->fp_instructions, struct fp_instr, link);
   assert(batch->tex_word && batch->alu_sched.num_instructions == 0);
   struct fp_alu_instr_packet *alu = CALLOC_STRUCT(fp_alu_instr_packet);
   assert(alu);
   list_inithead(&alu->link);

   unsigned swizzle[4] = {
      factor->SwizzleX, factor->SwizzleY,
      factor->SwizzleZ, factor->SwizzleW
   };

   for (unsigned i = 0; i < 4; i++) {
      unsigned component = i < 3 ? 2 - i : 3;
      unsigned uniform_index = factor->Index * 4 + swizzle[i];
      assert(uniform_index < 32);

      struct fp_alu_src_operand sample = {
         .index = component / 2,
         .datatype = FP_DATATYPE_FIXED10,
         .sub_reg_select_high = component % 2
      };
      if (i == 3 && fp->simple_tex_alpha_one)
         sample = fp_alu_src_one();


      struct fp_alu_src_operand scale = {
         .index = 32 + uniform_index,
         .datatype = FP_DATATYPE_FP20
      };

      alu->slots[i] = (struct fp_alu_instr) {
         .op = FP_ALU_OP_MAD,
         .dst = fp_alu_dst(dst, i, inst->Instruction.Saturate != 0),
         .src = { sample, scale, fp_alu_src_zero(), fp_alu_src_one() }
      };
   }

   batch->alu_sched.address = list_length(&fp->alu_instructions);
   batch->alu_sched.num_instructions = 1;
   batch->dw.enable = 1;
   batch->dw.index = 1;
   batch->dw.src_regs = FP_DW_REGS_R2_R3;
   list_addtail(&alu->link, &fp->alu_instructions);
   fp->simple_coord_mask = 0;
   fp->simple_tex_temp = -1;
}


static void
emit_simple_tex_mov(struct grate_fp_shader *fp,
                    const struct tgsi_full_instruction *inst)
{
   const struct tgsi_dst_register *dst = &inst->Dst[0].Register;
   const struct tgsi_src_register *src = &inst->Src[0].Register;

   assert(dst->File == TGSI_FILE_TEMPORARY &&
          dst->Index == fp->simple_tex_temp);
   assert(!inst->Instruction.Saturate &&
          !src->Negate && !src->Absolute);

   if (dst->WriteMask == 0x7 &&
       src->File == TGSI_FILE_TEMPORARY &&
       src->Index == fp->simple_tex_temp &&
       src->SwizzleX == TGSI_SWIZZLE_X &&
       src->SwizzleY == TGSI_SWIZZLE_Y &&
       src->SwizzleZ == TGSI_SWIZZLE_Z) {
      /* RGB self-copy leaves the sampled channels unchanged. */
      return;
   }

   assert(dst->WriteMask == 0x8 &&
          src->File == TGSI_FILE_IMMEDIATE &&
          src->Index == 0 &&
          fp->simple_num_immediates == 1);
   assert(fp->simple_immediate[src->SwizzleW] == 0x3f800000u);
   fp->simple_tex_alpha_one = 1;
}

static void
emit_tgsi_instr(struct grate_fp_shader *fp, const struct tgsi_full_instruction *inst)
{
   bool saturate = inst->Instruction.Saturate != 0;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
      if (fp->simple_tex_temp >= 0) {
         emit_simple_tex_mov(fp, inst);
         break;
      }
      emit_vMOV(fp, &inst->Dst[0].Register, saturate,
                    &inst->Src[0].Register);
      break;
   case TGSI_OPCODE_TXP:
   case TGSI_OPCODE_TEX:
      emit_simple_tex(fp, inst);
      break;
   case TGSI_OPCODE_MUL:
      emit_simple_tex_mul(fp, inst);
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
   /* All metadata starts defined, including rejected/unverified paths. */
   memset(fp, 0, sizeof(*fp));
   list_inithead(&fp->fp_instructions);
   list_inithead(&fp->alu_instructions);
   list_inithead(&fp->mfu_instructions);

   fp->simple_tex_temp = -1;
   fp->simple_tex_alpha_one = 0;
   fp->simple_num_immediates = 0;
   fp->info.num_inputs = 0;
   fp->info.color_input = -1;
   fp->info.max_tram_row = 1;

   while (!tgsi_parse_end_of_tokens(tgsi)) {
      tgsi_parse_token(tgsi);
      switch (tgsi->FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         emit_tgsi_declaration(fp, &tgsi->FullToken.FullDeclaration);
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE: {
         const struct tgsi_full_immediate *imm =
            &tgsi->FullToken.FullImmediate;
         assert(fp->simple_num_immediates == 0);
         assert(imm->Immediate.DataType == TGSI_IMM_FLOAT32 &&
                imm->Immediate.NrTokens == 5);
         for (unsigned i = 0; i < 4; i++)
            fp->simple_immediate[i] = imm->u[i].Uint;
         fp->simple_num_immediates = 1;
         break;
      }

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (tgsi->FullToken.FullInstruction.Instruction.Opcode != TGSI_OPCODE_END)
            emit_tgsi_instr(fp, &tgsi->FullToken.FullInstruction);
         break;
      }
   }

   /*
    * Keep two output ALU packets in one pipeline batch when the
    * second packet needs no MFU or texture operation.
    */
   if (list_length(&fp->fp_instructions) == 2) {
      struct fp_instr *a = NULL, *b = NULL;
      list_for_each_entry(struct fp_instr, instr,
                          &fp->fp_instructions, link) {
         if (!a)
            a = instr;
         else
            b = instr;
      }

      if (a && b &&
          !a->tex_word && !b->tex_word &&
          a->dw.enable && b->dw.enable &&
          a->dw.index == b->dw.index &&
          a->dw.src_regs == b->dw.src_regs &&
          a->alu_sched.num_instructions == 1 &&
          b->alu_sched.num_instructions == 1 &&
          a->alu_sched.address == 0 &&
          b->alu_sched.address == 1 &&
          b->mfu_sched.num_instructions == 0) {
         a->alu_sched.num_instructions = 2;
         a->dw = b->dw;
         list_del(&b->link);
         FREE(b);
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
