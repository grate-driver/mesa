#ifndef TEGRA_PROGRAM_H
#define TEGRA_PROGRAM_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "tegra_compiler.h"

struct tegra_shader_blob {
   uint32_t *commands;
   int num_commands;
};

struct tegra_vertex_shader_state {
   struct pipe_shader_state base;
   struct tegra_shader_blob blob;
   uint16_t output_mask;
};

struct tegra_fragment_shader_state {
   struct pipe_shader_state base;
   struct tegra_shader_blob blob;
   struct tegra_fp_info info;
};

void
tegra_context_program_init(struct pipe_context *pcontext);

#endif
