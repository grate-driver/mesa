#include "fpir.h"

void
grate_fp_pack_alu(uint32_t *dst, struct fp_alu_instr *instr)
{
   union {
      struct __attribute__((packed)) {
         unsigned rD_fixed10:1;
         unsigned rD_absolute_value:1;
         unsigned rD_enable:1;
         unsigned rD_minus_one:1;
         unsigned rD_sub_reg_select_high:1;
         unsigned rD_reg_select:1;

         unsigned rC_scale_by_two:1;
         unsigned rC_negate:1;
         unsigned rC_absolute_value:1;
         unsigned rC_fixed10:1;
         unsigned rC_minus_one:1;
         unsigned rC_sub_reg_select_high:1;
         unsigned rC_reg_select:7;

         unsigned rB_scale_by_two:1;
         unsigned rB_negate:1;
         unsigned rB_absolute_value:1;
         unsigned rB_fixed10:1;
         unsigned rB_minus_one:1;
         unsigned rB_sub_reg_select_high:1;
         unsigned rB_reg_select:7;

         unsigned rA_scale_by_two:1;
         unsigned rA_negate:1;
         unsigned rA_absolute_value:1;
         unsigned rA_fixed10:1;
         unsigned rA_minus_one:1;
         unsigned rA_sub_reg_select_high:1;
         unsigned rA_reg_select:7;

         unsigned write_low_sub_reg:1;
         unsigned write_high_sub_reg:1;
         unsigned dst_reg:7;
         unsigned condition_code:2;
         unsigned saturate_result:1;
         unsigned scale_result:2;

         unsigned addition_disable:1;
         unsigned accumulate_result_this:1;
         unsigned accumulate_result_other:1;
         unsigned opcode:2;
      };

      uint32_t words[2];
   } tmp = {
      .opcode = instr->op,
      .dst_reg = instr->dst.index,
      .saturate_result = instr->dst.saturate,

      .write_low_sub_reg = instr->dst.write_low_sub_reg,
      .write_high_sub_reg = instr->dst.write_high_sub_reg,

      .rA_reg_select = instr->src[0].index,
      .rA_fixed10 = instr->src[0].datatype != FP_DATATYPE_FP20,
      .rA_sub_reg_select_high = instr->src[0].sub_reg_select_high,

      .rB_reg_select = instr->src[1].index,
      .rB_fixed10 = instr->src[1].datatype != FP_DATATYPE_FP20,
      .rB_sub_reg_select_high = instr->src[1].sub_reg_select_high,

      .rC_reg_select = instr->src[2].index,
      .rC_fixed10 = instr->src[2].datatype != FP_DATATYPE_FP20,
      .rC_sub_reg_select_high = instr->src[2].sub_reg_select_high,

      .rD_reg_select = instr->src[3].index == instr->src[2].index,
      .rD_fixed10 = instr->src[3].datatype != FP_DATATYPE_FP20,
      .rD_sub_reg_select_high = instr->src[3].sub_reg_select_high,
   };

   /* Tegra114 uses these bits for extra opcodes when rD is disabled */
   if (!tmp.rD_reg_select) {
      tmp.rD_fixed10 = 0;
      tmp.rD_absolute_value = 0;
      tmp.rD_enable = 0;
      tmp.rD_minus_one = 0;
      tmp.rD_sub_reg_select_high = 0;
   }

   /* copy packed instruction into destination */
   for (int i = 0; i < 2; ++i)
      dst[i] = tmp.words[1 - i];
}

uint32_t
grate_fp_pack_dw(struct fp_dw_instr *instr)
{
   union {
      struct __attribute__((packed)) {
         unsigned enable:1;
         unsigned unk_1:1;
         unsigned render_target_index:4;
         unsigned unk_6_9:4;
         unsigned stencil_write:1;
         unsigned unk_11_14:4;
         unsigned src_regs_select:1;
         unsigned unk_16_31:16;
      };

      uint32_t word;
   } tmp = {
      .enable = instr->enable,
      .unk_16_31 = instr->enable ? 2 : 0, // no idea what this is
      .render_target_index = instr->index,
      .stencil_write = instr->stencil_write,
      .src_regs_select = instr->src_regs,
   };

   return tmp.word;
}

void
grate_fp_pack_mfu(uint32_t *dst, struct fp_mfu_instr *instr)
{
   union {
      struct __attribute__((packed)) {
         unsigned var0_saturate:1;
         unsigned var0_opcode:2;
         unsigned var0_source:4;

         unsigned var1_saturate:1;
         unsigned var1_opcode:2;
         unsigned var1_source:4;

         unsigned var2_saturate:1;
         unsigned var2_opcode:2;
         unsigned var2_source:4;

         unsigned var3_saturate:1;
         unsigned var3_opcode:2;
         unsigned var3_source:4;

         unsigned __pad:4;

         unsigned mul0_src0:4;
         unsigned mul0_src1:4;
         unsigned mul0_dst:3;

         unsigned mul1_src0:4;
         unsigned mul1_src1:4;
         unsigned mul1_dst:3;

         unsigned opcode:4;
         unsigned reg:6;
      };

      uint32_t words[2];
   } tmp = {
      .opcode = instr->sfu.op,
      .reg = instr->sfu.reg,

      .mul0_src0 = instr->mul[0].src[0],
      .mul0_src1 = instr->mul[0].src[1],
      .mul0_dst = instr->mul[0].dst,

      .mul1_src0 = instr->mul[1].src[0],
      .mul1_src1 = instr->mul[1].src[1],
      .mul1_dst = instr->mul[1].dst,

      .var0_saturate = instr->var[0].saturate,
      .var0_opcode = instr->var[0].op,
      .var0_source = instr->var[0].tram_row,

      .var1_saturate = instr->var[1].saturate,
      .var1_opcode = instr->var[1].op,
      .var1_source = instr->var[1].tram_row,

      .var2_saturate = instr->var[2].saturate,
      .var2_opcode = instr->var[2].op,
      .var2_source = instr->var[2].tram_row,

      .var3_saturate = instr->var[3].saturate,
      .var3_opcode = instr->var[3].op,
      .var3_source = instr->var[3].tram_row,
   };

   /* copy packed instruction into destination */
   for (int i = 0; i < 2; ++i)
      dst[i] = tmp.words[1 - i];
}

uint32_t
grate_fp_pack_sched(struct fp_sched *sched)
{
   assert(sched->num_instructions >= 0 && sched->num_instructions < 4);
   assert(sched->address >= 0 && sched->address < 64);
   union {
      struct __attribute__((packed)) {
         unsigned num_instructions : 2;
         unsigned address : 6;
      };
      uint32_t word;
   } tmp = {
      .num_instructions = sched->num_instructions,
      .address = sched->address
   };
   return tmp.word;
}

uint32_t
grate_fp_pack_alu_sched_t114(struct fp_sched *sched)
{
   assert(sched->num_instructions >= 0 && sched->num_instructions < 4);
   assert(sched->address >= 0 && sched->address < 64);
   union {
      struct __attribute__((packed)) {
         unsigned __pad:8;
         unsigned num_instructions:8;
         unsigned address:8;
         unsigned unk24_31:8;
      };
      uint32_t word;
   } tmp = {
      .num_instructions = sched->num_instructions,
      .address = sched->address,
      .unk24_31 = 0xe0,
   };
   return tmp.word;
}
