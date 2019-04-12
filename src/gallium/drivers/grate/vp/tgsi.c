#include "grate_compiler.h"
#include "vpir.h"

#include "tgsi/tgsi_parse.h"

#include "util/u_memory.h"

static struct vp_src_operand
src_undef()
{
   struct vp_src_operand ret = {
      .file = VP_SRC_FILE_UNDEF,
      .index = 0,
      .swizzle = { VP_SWZ_X, VP_SWZ_Y, VP_SWZ_Z, VP_SWZ_W }
   };
   return ret;
}

static struct vp_src_operand
attrib(int index, const enum vp_swz swizzle[4], bool negate, bool absolute)
{
   struct vp_src_operand ret = {
      .file = VP_SRC_FILE_ATTRIB,
      .index = index,
      .negate = negate,
      .absolute = absolute
   };
   memcpy(ret.swizzle, swizzle, sizeof(ret.swizzle));
   return ret;
}

static struct vp_src_operand
uniform(int index, const enum vp_swz swizzle[4], bool negate, bool absolute)
{
   struct vp_src_operand ret = {
      .file = VP_SRC_FILE_UNIFORM,
      .index = index,
      .negate = negate,
      .absolute = absolute
   };
   memcpy(ret.swizzle, swizzle, sizeof(ret.swizzle));
   return ret;
}

static struct vp_src_operand
src_temp(int index, const enum vp_swz swizzle[4], bool negate, bool absolute)
{
   struct vp_src_operand ret = {
      .file = VP_SRC_FILE_TEMP,
      .index = index,
      .negate = negate,
      .absolute = absolute
   };
   memcpy(ret.swizzle, swizzle, sizeof(ret.swizzle));
   return ret;
}

static struct vp_dst_operand
dst_undef()
{
   struct vp_dst_operand ret = {
      .file = VP_DST_FILE_UNDEF,
      .index = 0,
      .write_mask = 0,
      .saturate = 0
   };
   return ret;
}

static struct vp_dst_operand
emit_output(struct grate_vp_shader *vp, int index,
            unsigned int write_mask, bool saturate)
{
   vp->output_mask |= 1 << index;
   struct vp_dst_operand ret = {
      .file = VP_DST_FILE_OUTPUT,
      .index = index,
      .write_mask = write_mask,
      .saturate = saturate
   };
   return ret;
}

static struct vp_dst_operand
dst_temp(int index, unsigned int write_mask, bool saturate)
{
   struct vp_dst_operand ret = {
      .file = VP_DST_FILE_TEMP,
      .index = index,
      .write_mask = write_mask,
      .saturate = saturate
   };
   return ret;
}

static struct vp_vec_instr
emit_vec_unop(enum vp_vec_op op, struct vp_dst_operand dst,
              struct vp_src_operand src)
{
   struct vp_vec_instr ret = {
      .op = op,
      .dst = dst,
      .src = { src, src_undef(), src_undef() }
   };
   return ret;
}

static struct vp_vec_instr
emit_vec_binop(enum vp_vec_op op, struct vp_dst_operand dst,
              struct vp_src_operand src0, struct vp_src_operand src1)
{
   struct vp_vec_instr ret = {
      .op = op,
      .dst = dst,
      .src = { src0, src1, src_undef() }
   };
   return ret;
}

static struct vp_vec_instr
emit_vNOP()
{
   struct vp_vec_instr ret = {
      .op = VP_VEC_OP_NOP,
      .dst = dst_undef(),
      .src = { src_undef(), src_undef(), src_undef() }
   };
   return ret;
}

static struct vp_vec_instr
emit_vMOV(struct vp_dst_operand dst, struct vp_src_operand src)
{
   return emit_vec_unop(VP_VEC_OP_MOV, dst, src);
}

static struct vp_vec_instr
emit_vADD(struct vp_dst_operand dst, struct vp_src_operand src0,
          struct vp_src_operand src2)
{
   struct vp_vec_instr ret = {
      .op = VP_VEC_OP_ADD,
      .dst = dst,
      .src = { src0, src_undef(), src2 } // add is "strange" in that it takes src0 and src2
   };
   return ret;
}

#define GEN_V_BINOP(OP) \
static struct vp_vec_instr \
emit_v ## OP (struct vp_dst_operand dst, struct vp_src_operand src0, \
          struct vp_src_operand src1) \
{ \
   return emit_vec_binop(VP_VEC_OP_ ## OP, dst, src0, src1); \
}

GEN_V_BINOP(MUL)
GEN_V_BINOP(DP3)
GEN_V_BINOP(DP4)
GEN_V_BINOP(SLT)
GEN_V_BINOP(MAX)

static struct vp_vec_instr
emit_vMAD(struct vp_dst_operand dst, struct vp_src_operand src0,
          struct vp_src_operand src1, struct vp_src_operand src2)
{
   struct vp_vec_instr ret = {
      .op = VP_VEC_OP_MAD,
      .dst = dst,
      .src = { src0, src1, src2 }
   };
   return ret;
}

static struct vp_scalar_instr
emit_sNOP()
{
   struct vp_scalar_instr ret = {
      .op = VP_SCALAR_OP_NOP,
      .dst = dst_undef(),
      .src = src_undef()
   };
   return ret;
}

#define GEN_S_UNOP(OP) \
static struct vp_scalar_instr \
emit_s ## OP (struct vp_dst_operand dst, struct vp_src_operand src) \
{ \
   struct vp_scalar_instr ret = { \
      .op = VP_SCALAR_OP_ ## OP, \
      .dst = dst, \
      .src = src \
   }; \
   return ret; \
}

GEN_S_UNOP(RSQ)

static struct vp_instr *
emit_packed(struct vp_vec_instr vec, struct vp_scalar_instr scalar)
{
   struct vp_instr *ret = CALLOC_STRUCT(vp_instr);
   list_inithead(&ret->link);
   ret->vec = vec;
   ret->scalar = scalar;
   return ret;
}

static struct vp_dst_operand
tgsi_dst_to_vp(struct grate_vp_shader *vp, const struct tgsi_dst_register *dst, bool saturate)
{
   switch (dst->File) {
   case TGSI_FILE_OUTPUT:
      return emit_output(vp, dst->Index, dst->WriteMask, saturate);

   case TGSI_FILE_TEMPORARY:
      return dst_temp(dst->Index, dst->WriteMask, saturate);

   default:
      unreachable("unsupported output");
   }
}

static struct vp_src_operand
tgsi_src_to_vp(struct grate_vp_shader *vp, const struct tgsi_src_register *src)
{
   enum vp_swz swizzle[4] = {
      src->SwizzleX,
      src->SwizzleY,
      src->SwizzleZ,
      src->SwizzleW
   };
   bool negate = src->Negate != 0;
   bool absolute = src->Absolute != 0;

   switch (src->File) {
   case TGSI_FILE_INPUT:
      return attrib(src->Index, swizzle, negate, absolute);

   case TGSI_FILE_CONSTANT:
      return uniform(src->Index, swizzle, negate, absolute);

   case TGSI_FILE_TEMPORARY:
      return src_temp(src->Index, swizzle, negate, absolute);

   case TGSI_FILE_IMMEDIATE:
      /* HACK: allocate uniforms from the top for immediates; need to actually record these */
      return uniform(1023 - src->Index, swizzle, negate, absolute);

   default:
      unreachable("unsupported input!");
   }
}

static struct vp_instr *
tgsi_to_vp(struct grate_vp_shader *vp, const struct tgsi_full_instruction *inst)
{
   bool saturate = inst->Instruction.Saturate != 0;
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
      return emit_packed(emit_vMOV(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_ADD:
      return emit_packed(emit_vADD(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_MUL:
      return emit_packed(emit_vMUL(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_DP3:
      return emit_packed(emit_vDP3(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_DP4:
      return emit_packed(emit_vDP4(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_SLT:
      return emit_packed(emit_vSLT(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_MAX:
      return emit_packed(emit_vMAX(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_MAD:
      return emit_packed(emit_vMAD(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[1].Register),
                                   tgsi_src_to_vp(vp, &inst->Src[2].Register)),
                         emit_sNOP());

   case TGSI_OPCODE_RSQ:
      return emit_packed(emit_vNOP(),
                         emit_sRSQ(tgsi_dst_to_vp(vp, &inst->Dst[0].Register, saturate),
                                   tgsi_src_to_vp(vp, &inst->Src[0].Register)));

   default:
      unreachable("unsupported TGSI-opcode!");
   }
}

void
grate_tgsi_to_vp(struct grate_vp_shader *vp, struct tgsi_parse_context *tgsi)
{
   list_inithead(&vp->instructions);
   vp->output_mask = 0;

   while (!tgsi_parse_end_of_tokens(tgsi)) {
      tgsi_parse_token(tgsi);
      switch (tgsi->FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (tgsi->FullToken.FullInstruction.Instruction.Opcode != TGSI_OPCODE_END) {
            struct vp_instr *instr = tgsi_to_vp(vp, &tgsi->FullToken.FullInstruction);
            list_addtail(&instr->link, &vp->instructions);
         }
         break;
      }
   }
}
