#ifndef VPIR_H
#define VPIR_H

#include "util/list.h"

#include "stdbool.h"
#include "stdint.h"

enum vp_src_file {
   VP_SRC_FILE_UNDEF = 0,
   VP_SRC_FILE_TEMP = 1,
   VP_SRC_FILE_ATTRIB = 2,
   VP_SRC_FILE_UNIFORM = 3,
};

enum vp_dst_file {
   VP_DST_FILE_TEMP,
   VP_DST_FILE_OUTPUT,
   VP_DST_FILE_UNDEF
};

enum vp_swz {
   VP_SWZ_X = 0,
   VP_SWZ_Y = 1,
   VP_SWZ_Z = 2,
   VP_SWZ_W = 3
};

enum vp_vec_op {
   VP_VEC_OP_NOP = 0,
   VP_VEC_OP_MOV = 1,
   VP_VEC_OP_MUL = 2,
   VP_VEC_OP_ADD = 3,
   VP_VEC_OP_MAD = 4,
   VP_VEC_OP_DP3 = 5,
   VP_VEC_OP_DPH = 6,
   VP_VEC_OP_DP4 = 7,
   VP_VEC_OP_DST = 8,
   VP_VEC_OP_MIN = 9,
   VP_VEC_OP_MAX = 10,
   VP_VEC_OP_SLT = 11,
   VP_VEC_OP_SGE = 12,
   VP_VEC_OP_ARL = 13,
   VP_VEC_OP_FRC = 14,
   VP_VEC_OP_FLR = 15,
   VP_VEC_OP_SEQ = 16,
   VP_VEC_OP_SFL = 17,
   VP_VEC_OP_SGT = 18,
   VP_VEC_OP_SLE = 19,
   VP_VEC_OP_SNE = 20,
   VP_VEC_OP_STR = 21,
   VP_VEC_OP_SSG = 22,
   VP_VEC_OP_ARR = 23,
   VP_VEC_OP_ARA = 24,
   VP_VEC_OP_TXL = 25,
   VP_VEC_OP_PUSHA = 26,
   VP_VEC_OP_POPA = 27
};

enum vp_scalar_op {
   VP_SCALAR_OP_NOP = 0,
   VP_SCALAR_OP_MOV = 1,
   VP_SCALAR_OP_RCP = 2,
   VP_SCALAR_OP_RCC = 3,
   VP_SCALAR_OP_RSQ = 4,
   VP_SCALAR_OP_EXP = 5,
   VP_SCALAR_OP_LOG = 6,
   VP_SCALAR_OP_LIT = 7,
   VP_SCALAR_OP_BRA = 9,
   VP_SCALAR_OP_CAL = 11,
   VP_SCALAR_OP_RET = 12,
   VP_SCALAR_OP_LG2 = 13,
   VP_SCALAR_OP_EX2 = 14,
   VP_SCALAR_OP_SIN = 15,
   VP_SCALAR_OP_COS = 16,
   VP_SCALAR_OP_PUSHA = 19,
   VP_SCALAR_OP_POPA = 20
};

struct vp_dst_operand {
   enum vp_dst_file file;
   int index;
   unsigned int write_mask;
   bool saturate;
};

struct vp_src_operand {
   enum vp_src_file file;
   int index;
   enum vp_swz swizzle[4];
   bool negate, absolute;
};

struct vp_vec_instr {
   enum vp_vec_op op;
   struct vp_dst_operand dst;
   struct vp_src_operand src[3];
};

struct vp_scalar_instr {
   enum vp_scalar_op op;
   struct vp_dst_operand dst;
   struct vp_src_operand src;
};

struct vp_instr {
   struct list_head link;
   struct vp_vec_instr vec;
   struct vp_scalar_instr scalar;
};

void
grate_vp_pack(uint32_t *dst, struct vp_instr *instr, bool end_of_program);

#endif
