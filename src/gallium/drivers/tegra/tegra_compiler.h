#ifndef TEGRA_COMPILER_H
#define TEGRA_COMPILER_H

#include "util/list.h"

#include <stdint.h>

struct tgsi_parse_context;

struct tegra_vpe_shader {
   struct list_head instructions;
   uint16_t output_mask;
};

void
tegra_tgsi_to_vpe(struct tegra_vpe_shader *vpe, struct tgsi_parse_context *tgsi);

#endif
