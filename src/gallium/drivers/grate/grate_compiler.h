#ifndef GRATE_COMPILER_H
#define GRATE_COMPILER_H

#include "util/list.h"

#include <stdint.h>

struct tgsi_parse_context;

struct grate_vpe_shader {
   struct list_head instructions;
   uint16_t output_mask;
};

struct grate_fp_shader {
   struct list_head fp_instructions;
   struct list_head alu_instructions;
   struct list_head mfu_instructions;
};

void
grate_tgsi_to_vpe(struct grate_vpe_shader *vpe, struct tgsi_parse_context *tgsi);

void
grate_tgsi_to_fp(struct grate_fp_shader *fp, struct tgsi_parse_context *tgsi);

#endif
