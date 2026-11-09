#ifndef GRATE_COMPILER_H
#define GRATE_COMPILER_H

#include "util/list.h"

#include <stdint.h>

struct tgsi_parse_context;

#define GRATE_VP_MAX_CONST_REGS 256

struct grate_vp_shader {
   struct list_head instructions;
   uint16_t output_mask;
   unsigned immediate_base;
   unsigned num_immediates;
   uint32_t immediates[GRATE_VP_MAX_CONST_REGS][4];
};

struct grate_fp_info {
   struct {
      uint32_t src;
      uint32_t dst;
   } inputs[16];
   int num_inputs;
   int color_input;
   int max_tram_row;
};

struct grate_fp_shader {
   int simple_tex_temp;
   unsigned simple_tex_alpha_one;
   unsigned simple_num_immediates;
   uint32_t simple_immediate[4];
   struct list_head fp_instructions;
   struct list_head alu_instructions;
   struct list_head mfu_instructions;
   struct grate_fp_info info;
};

void
grate_tgsi_to_vp(struct grate_vp_shader *vp, struct tgsi_parse_context *tgsi);

void
grate_tgsi_to_fp(struct grate_fp_shader *fp, struct tgsi_parse_context *tgsi);

#endif
