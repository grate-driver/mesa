#ifndef FP_IR_H
#define FP_IR_H

#include "util/list.h"

#include "stdbool.h"
#include "stdint.h"

enum fp_alu_op {
   FP_ALU_OP_MAD = 0,
   FP_ALU_OP_MIN = 1,
   FP_ALU_OP_MAX = 2,
   FP_ALU_OP_CSEL = 3
};

enum fp_scale {
   FP_SCALE_NONE = 0,
   FP_SCALE_MUL2 = 1,
   FP_SCALE_MUL4 = 2,
   FP_SCALE_DIV2 = 3
};

enum fp_condition {
   FP_CONDITION_ALWAYS = 0,
   FP_CONDITION_EQUAL = 1,
   FP_CONDITION_GEQUAL = 2,
   FP_CONDITION_GREATER = 3
};

struct fp_alu_dst_operand {
   bool write_low_sub_reg;
   bool write_high_sub_reg;
   unsigned index;
   bool saturate;
};

enum fp_datatype {
   FP_DATATYPE_FP20 = 0,
   FP_DATATYPE_FIXED10 = 1
};

struct fp_alu_src_operand {
   bool scale_by_two;
   bool negate;
   bool absolute_value;
   enum fp_datatype datatype;
   bool minus_one;
   bool sub_reg_select_high;
   unsigned index;
};

struct fp_alu_instr {
   enum fp_condition condition;

   enum fp_alu_op op;
   enum fp_scale scale;

   struct fp_alu_dst_operand dst;
   struct fp_alu_src_operand src[4];
};

enum fp_dw_src_regs {
   FP_DW_REGS_R0_R1 = 0,
   FP_DW_REGS_R2_R3 = 1
};

struct fp_dw_instr {
   bool enable;
   int index;
   bool stencil_write;
   enum fp_dw_src_regs src_regs;
};

enum fp_sfu_op {
   FP_SFU_OP_NOP = 0,
   FP_SFU_OP_RCP = 1,
   FP_SFU_OP_RSQ = 2,
   FP_SFU_OP_LG2 = 3,
   FP_SFU_OP_EX2 = 4,
   FP_SFU_OP_SQRT = 5,
   FP_SFU_OP_SIN = 6,
   FP_SFU_OP_COS = 7,
   FP_SFU_OP_FRC = 8,
   FP_SFU_OP_PREEX2 = 9,
   FP_SFU_OP_PRESIN = 10,
   FP_SFU_OP_PRECOS = 11
};

enum fp_mfu_mul_dst {
   FP_MFU_MUL_DST_BARYCENTRIC_WEIGHT = 1,
   FP_MFU_MUL_DST_ROW_REG_0 = 4,
   FP_MFU_MUL_DST_ROW_REG_1 = 5,
   FP_MFU_MUL_DST_ROW_REG_2 = 6,
   FP_MFU_MUL_DST_ROW_REG_3 = 7
};

enum fp_mfu_mul_src {
   FP_MFU_MUL_SRC_ROW_REG_0 = 0,
   FP_MFU_MUL_SRC_ROW_REG_1 = 1,
   FP_MFU_MUL_SRC_ROW_REG_2 = 2,
   FP_MFU_MUL_SRC_ROW_REG_3 = 3,
   FP_MFU_MUL_SRC_SFU_RESULT = 10,
   FP_MFU_MUL_SRC_BARYCENTRIC_COEF_0 = 11,
   FP_MFU_MUL_SRC_BARYCENTRIC_COEF_1 = 12,
   FP_MFU_MUL_SRC_CONST_1 = 13,
};

struct fp_mfu_mul {
   enum fp_mfu_mul_dst dst;
   enum fp_mfu_mul_src src[2];
};

enum fp_var_op {
   FP_VAR_OP_NOP = 0,
   FP_VAR_OP_FP20 = 1,
   FP_VAR_OP_FX10 = 2,
};

struct fp_var_instr {
   bool saturate;
   enum fp_var_op op;
   unsigned tram_row;
};

struct fp_sfu_instr {
   enum fp_sfu_op op;
   unsigned reg;
};

struct fp_mfu_instr {
   struct list_head link;
   struct fp_sfu_instr sfu;
   struct fp_mfu_mul mul[2];
   struct fp_var_instr var[4];
};

struct fp_alu_instr_packet {
   struct list_head link;
   struct fp_alu_instr slots[4];
};

struct fp_sched {
   int num_instructions;
   int address;
};

struct fp_instr {
   struct list_head link;
   // TODO: PSEQ
   struct fp_sched mfu_sched;
   // TODO: TEX
   struct fp_sched alu_sched;
   struct fp_dw_instr dw;
};

void
grate_fp_pack_alu(uint32_t *dst, struct fp_alu_instr *instr);

uint32_t
grate_fp_pack_dw(struct fp_dw_instr *instr);

void
grate_fp_pack_mfu(uint32_t *dst, struct fp_mfu_instr *instr);

uint32_t
grate_fp_pack_sched(struct fp_sched *sched);

#endif
