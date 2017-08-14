#include <assert.h>
#include <stdint.h>

#include "util/macros.h"

#include "grate_vpe_ir.h"

static unsigned
vpe_write_mask(unsigned input)
{
   return (((input >> 0) & 1) << 3) |
          (((input >> 1) & 1) << 2) |
          (((input >> 2) & 1) << 1) |
          (((input >> 3) & 1) << 0);
}

static unsigned
vpe_swizzle(enum vpe_swz swizzle[4])
{
   return (swizzle[0] << 6) |
          (swizzle[1] << 4) |
          (swizzle[2] << 2) |
          (swizzle[3] << 0);
}

static unsigned
vpe_src_reg(struct vpe_src_operand op)
{
   union {
      struct __attribute__((packed)) {
         unsigned type : 2;    //  0 .. 1
         unsigned index : 6;   //  2 .. 7
         unsigned swizzle : 8; //  8 .. 15
         unsigned negate : 1;  //  16
      };
      unsigned value;
   } u;

   u.type = op.file;
   u.index = op.index;
   u.swizzle = vpe_swizzle(op.swizzle);
   u.negate = op.negate;

   return u.value;
}


void
grate_vpe_pack(uint32_t *dst, struct vpe_instr *instr, bool end_of_program)
{
   /* we can only handle one output register per instruction */
   assert(instr->vec.dst.file != VPE_DST_FILE_OUTPUT ||
          instr->scalar.dst.file != VPE_DST_FILE_OUTPUT);

   union {
      struct __attribute__((packed)) {
         unsigned end_of_program : 1;                       //   0
         unsigned constant_relative_addressing_enable : 1;  //   1
         unsigned export_write_index : 5;                   //   2 .. 6
         unsigned scalar_rD_index : 6;                      //   7 .. 12
         unsigned vector_op_write_mask : 4;                 //  13 .. 16
         unsigned scalar_op_write_mask : 4;                 //  17 .. 20
         unsigned rC : 17;                                  //  21 .. 37
         unsigned rB : 17;                                  //  38 .. 54
         unsigned rA : 17;                                  //  55 .. 71
         unsigned attribute_fetch_index : 4;                //  72 .. 75
         unsigned uniform_fetch_index : 10;                 //  76 .. 85
         unsigned vector_opcode : 5;                        //  86 .. 90
         unsigned scalar_opcode : 5;                        //  91 .. 95
         unsigned address_register_select : 2;              //  96 .. 97
         unsigned predicate_swizzle : 8;                    //  98 .. 105
         unsigned predicate_lt : 1;                         // 106
         unsigned predicate_eq : 1;                         // 107
         unsigned predicate_gt : 1;                         // 108
         unsigned condition_check : 1;                      // 109
         unsigned condition_set : 1;                        // 110
         unsigned vector_rD_index : 6;                      // 111 .. 116
         unsigned rA_absolute : 1;                          // 117
         unsigned rB_absolute : 1;                          // 118
         unsigned rC_absolute : 1;                          // 119
         unsigned bit120 : 1;                               // 120
         unsigned condition_register_index : 1;             // 121
         unsigned saturate_result : 1;                      // 122
         unsigned attribute_relative_addressing_enable : 1; // 123
         unsigned export_relative_addressing_enable : 1;    // 124
         unsigned condition_flags_write_enable : 1;         // 125
         unsigned export_vector_write_enable : 1;           // 126
         unsigned bit127 : 1;                               // 127
      };

      uint32_t words[4];
   } tmp = {
      .predicate_lt = 1,
      .predicate_eq = 1,
      .predicate_gt = 1,

      .predicate_swizzle = (0 << 6) | (1 << 4) | (2 << 2) | 3,
   };

   /* find the attribute/uniform fetch-values, and zero out the index
    * for these registers.
    */
   int attr_fetch = -1, uniform_fetch = -1;
   for (int i = 0; i < 3; ++i) {
      switch (instr->vec.src[i].file) {
      case VPE_SRC_FILE_ATTRIB:
         assert(attr_fetch < 0 ||
                attr_fetch == instr->vec.src[i].index);
         attr_fetch = instr->vec.src[i].index;
         instr->vec.src[i].index = 0;
         break;

      case VPE_SRC_FILE_UNIFORM:
         assert(uniform_fetch < 0 ||
                uniform_fetch == instr->vec.src[i].index);
         uniform_fetch = instr->vec.src[i].index;
         instr->vec.src[i].index = 0;
         break;

      default: /* nothing */
         break;
      }
   }

   switch (instr->scalar.src.file) {
   case VPE_SRC_FILE_ATTRIB:
      assert(attr_fetch < 0 ||
             attr_fetch == instr->scalar.src.index);
      attr_fetch = instr->scalar.src.index;
      instr->scalar.src.index = 0;
      break;

      case VPE_SRC_FILE_UNIFORM:
         assert(uniform_fetch < 0 ||
                uniform_fetch == instr->scalar.src.index);
         uniform_fetch = instr->scalar.src.index;
         instr->scalar.src.index = 0;
         break;

      default: /* nothing */
         break;

   }

   tmp.attribute_fetch_index = attr_fetch >= 0 ? attr_fetch : 0;
   tmp.uniform_fetch_index = uniform_fetch >= 0 ? uniform_fetch : 0;

   tmp.vector_opcode = instr->vec.op;
   switch (instr->vec.dst.file) {
   case VPE_DST_FILE_TEMP:
      tmp.vector_rD_index = instr->vec.dst.index;
      tmp.export_write_index = 31;
      break;

   case VPE_DST_FILE_OUTPUT:
      tmp.vector_rD_index = 63; // disable register-write
      tmp.export_vector_write_enable = 1;
      tmp.export_write_index = instr->vec.dst.index;
      break;

   case VPE_DST_FILE_UNDEF:
      // assert(0);  // TODO: consult NOP
      break;

   default:
      unreachable("illegal enum vpe_dst_file value");
   }
   tmp.vector_op_write_mask = vpe_write_mask(instr->vec.dst.write_mask);

   tmp.scalar_opcode = instr->scalar.op;
   switch (instr->scalar.dst.file) {
   case VPE_DST_FILE_TEMP:
      tmp.scalar_rD_index = instr->scalar.dst.index;
      tmp.export_write_index = 31;
      break;

   case VPE_DST_FILE_OUTPUT:
      tmp.scalar_rD_index = 63; // disable register-write
      tmp.export_vector_write_enable = 0;
      tmp.export_write_index = instr->scalar.dst.index;
      break;

   case VPE_DST_FILE_UNDEF:
      // assert(0);  // TODO: consult NOP
      break;

   default:
      unreachable("illegal enum vpe_dst_file value");
   }
   tmp.scalar_op_write_mask = vpe_write_mask(instr->scalar.dst.write_mask);

   tmp.rA = vpe_src_reg(instr->vec.src[0]);
   tmp.rA_absolute = instr->vec.src[0].absolute;

   tmp.rB = vpe_src_reg(instr->vec.src[1]);
   tmp.rB_absolute = instr->vec.src[1].absolute;

   if (instr->vec.src[2].file != VPE_SRC_FILE_UNDEF) {
      tmp.rC = vpe_src_reg(instr->vec.src[2]);
      tmp.rC_absolute = instr->vec.src[2].absolute;
   } else if (instr->scalar.src.file != VPE_SRC_FILE_UNDEF) {
      tmp.rC = vpe_src_reg(instr->scalar.src);
      tmp.rC_absolute = instr->scalar.src.absolute;
   }

   tmp.end_of_program = end_of_program;

   /* copy packed instruction into destination */
   for (int i = 0; i < 4; ++i)
      dst[i] = tmp.words[3 - i];
}
