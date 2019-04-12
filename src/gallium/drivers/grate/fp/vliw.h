#ifndef VLIW_H
#define VLIW_H

#include <stdint.h>

union fragment_mfu_instruction {
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

   struct __attribute__((packed)) {
      uint32_t part0;
      uint32_t part1;
   };
};

union fragment_alu_instruction {
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

   struct __attribute__((packed)) {
      uint32_t part0;
      uint32_t part1;
   };
};

union fragment_alu_instruction_packet {
   struct __attribute__((packed)) {
      union fragment_alu_instruction a[4];
   };

   union {
      struct __attribute__((packed)) {
         uint64_t __pad1;
         uint64_t __pad2;
         uint64_t __pad3;
         unsigned __pad4:4;
         unsigned fx10_low:10;
         unsigned fx10_high:10;
      };

      struct __attribute__((packed)) {
         uint64_t __pad5;
         uint64_t __pad6;
         uint64_t __pad7;
         unsigned __pad8:4;
         unsigned fp20:20;
      };
   } imm0;

   union {
      struct __attribute__((packed)) {
         uint64_t __pad1;
         uint64_t __pad2;
         uint64_t __pad3;
         unsigned __pad4:24;
         unsigned fx10_low:10;
         unsigned fx10_high:10;
      };

      struct __attribute__((packed)) {
         uint64_t __pad5;
         uint64_t __pad6;
         uint64_t __pad7;
         unsigned __pad8:24;
         unsigned fp20:20;
      };
   } imm1;

   union {
      struct __attribute__((packed)) {
         uint64_t __pad1;
         uint64_t __pad2;
         uint64_t __pad3;
         uint32_t __pad4;
         unsigned __pad5:12;
         unsigned fx10_low:10;
         unsigned fx10_high:10;
      };

      struct __attribute__((packed)) {
         uint64_t __pad6;
         uint64_t __pad7;
         uint64_t __pad8;
         uint32_t __pad9;
         unsigned __pad10:12;
         unsigned fp20:20;
      };
   } imm2;

   struct __attribute__((packed)) {
      uint32_t part0;
      uint32_t part1;
      uint32_t part2;
      uint32_t part3;
      uint32_t part4;
      uint32_t part5;
      uint32_t part6;
      uint32_t part7;

      uint32_t complement;
   };
};

#endif // FP_VLIW_H
