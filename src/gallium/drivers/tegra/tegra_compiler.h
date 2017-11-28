#ifndef TEGRA_COMPILER_H
#define TEGRA_COMPILER_H

#include "util/list.h"

#include <stdint.h>

struct tgsi_parse_context;

struct tegra_vpe_shader {
   struct list_head instructions;
   uint16_t output_mask;
};

struct tegra_fp_info {
   struct {
      uint32_t src;
      uint32_t dst;
   } inputs[16];
   int num_inputs;
   int color_input;
};

struct tegra_fp_shader {
   struct list_head fp_instructions;
   struct list_head alu_instructions;
   struct list_head mfu_instructions;
   struct tegra_fp_info info;
};

void
tegra_tgsi_to_vpe(struct tegra_vpe_shader *vpe, struct tgsi_parse_context *tgsi);

void
tegra_tgsi_to_fp(struct tegra_fp_shader *fp, struct tgsi_parse_context *tgsi);

#endif
