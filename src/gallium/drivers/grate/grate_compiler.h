#ifndef GRATE_COMPILER_H
#define GRATE_COMPILER_H

#include "util/list.h"

#include <stdint.h>

struct tgsi_parse_context;

struct grate_vpe_shader {
   struct list_head instructions;
   uint16_t output_mask;
};

void
grate_tgsi_to_vpe(struct grate_vpe_shader *vpe, struct tgsi_parse_context *tgsi);

#endif
