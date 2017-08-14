#ifndef GRATE_VPE_IR_H
#define GRATE_VPE_IR_H

#include "util/list.h"

#include "stdbool.h"
#include "stdint.h"

enum vpe_src_file {
   VPE_SRC_FILE_UNDEF = 0,
   VPE_SRC_FILE_TEMP = 1,
   VPE_SRC_FILE_ATTRIB = 2,
   VPE_SRC_FILE_UNIFORM = 3,
};

enum vpe_dst_file {
   VPE_DST_FILE_TEMP,
   VPE_DST_FILE_OUTPUT,
   VPE_DST_FILE_UNDEF
};

enum vpe_swz {
   VPE_SWZ_X = 0,
   VPE_SWZ_Y = 1,
   VPE_SWZ_Z = 2,
   VPE_SWZ_W = 3
};

enum vpe_vec_op {
   VPE_VEC_OP_NOP = 0,
   VPE_VEC_OP_MOV = 1,
   VPE_VEC_OP_MUL = 2,
   VPE_VEC_OP_ADD = 3,
   VPE_VEC_OP_MAD = 4,
   VPE_VEC_OP_DP3 = 5,
   VPE_VEC_OP_DPH = 6,
   VPE_VEC_OP_DP4 = 7,
   VPE_VEC_OP_DST = 8,
   VPE_VEC_OP_MIN = 9,
   VPE_VEC_OP_MAX = 10,
   VPE_VEC_OP_SLT = 11,
   VPE_VEC_OP_SGE = 12,
   VPE_VEC_OP_ARL = 13,
   VPE_VEC_OP_FRC = 14,
   VPE_VEC_OP_FLR = 15,
   VPE_VEC_OP_SEQ = 16,
   VPE_VEC_OP_SFL = 17,
   VPE_VEC_OP_SGT = 18,
   VPE_VEC_OP_SLE = 19,
   VPE_VEC_OP_SNE = 20,
   VPE_VEC_OP_STR = 21,
   VPE_VEC_OP_SSG = 22,
   VPE_VEC_OP_ARR = 23,
   VPE_VEC_OP_ARA = 24,
   VPE_VEC_OP_TXL = 25,
   VPE_VEC_OP_PUSHA = 26,
   VPE_VEC_OP_POPA = 27
};

enum vpe_scalar_op {
   VPE_SCALAR_OP_NOP = 0,
   VPE_SCALAR_OP_MOV = 1,
   VPE_SCALAR_OP_RCP = 2,
   VPE_SCALAR_OP_RCC = 3,
   VPE_SCALAR_OP_RSQ = 4,
   VPE_SCALAR_OP_EXP = 5,
   VPE_SCALAR_OP_LOG = 6,
   VPE_SCALAR_OP_LIT = 7,
   VPE_SCALAR_OP_BRA = 9,
   VPE_SCALAR_OP_CAL = 11,
   VPE_SCALAR_OP_RET = 12,
   VPE_SCALAR_OP_LG2 = 13,
   VPE_SCALAR_OP_EX2 = 14,
   VPE_SCALAR_OP_SIN = 15,
   VPE_SCALAR_OP_COS = 16,
   VPE_SCALAR_OP_PUSHA = 19,
   VPE_SCALAR_OP_POPA = 20
};

struct vpe_dst_operand {
   enum vpe_dst_file file;
   int index;
   unsigned int write_mask;
   bool saturate;
};

struct vpe_src_operand {
   enum vpe_src_file file;
   int index;
   enum vpe_swz swizzle[4];
   bool negate, absolute;
};

struct vpe_vec_instr {
   enum vpe_vec_op op;
   struct vpe_dst_operand dst;
   struct vpe_src_operand src[3];
};

struct vpe_scalar_instr {
   enum vpe_scalar_op op;
   struct vpe_dst_operand dst;
   struct vpe_src_operand src;
};

struct vpe_instr {
   struct list_head link;
   struct vpe_vec_instr vec;
   struct vpe_scalar_instr scalar;
};

void
grate_vpe_pack(uint32_t *dst, struct vpe_instr *instr, bool end_of_program);

#endif
