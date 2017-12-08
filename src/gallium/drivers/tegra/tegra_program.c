#include <stdio.h>
#include <string.h>

#include "util/u_dynarray.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "host1x01_hardware.h"
#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_screen.h"
#include "tegra_program.h"
#include "tegra_compiler.h"
#include "tgr_3d.xml.h"

#include "vpe_ir.h"
#include "fp_ir.h"

static void *
tegra_create_vs_state(struct pipe_context *pcontext,
                      const struct pipe_shader_state *template)
{
   struct tegra_vertex_shader_state *so =
      CALLOC_STRUCT(tegra_vertex_shader_state);

   if (!so)
      return NULL;

   so->base = *template;

   if (tegra_debug & TEGRA_DEBUG_TGSI) {
      fprintf(stderr, "DEBUG: TGSI:\n");
      tgsi_dump(template->tokens, 0);
      fprintf(stderr, "\n");
   }

   struct tgsi_parse_context parser;
   unsigned ok = tgsi_parse_init(&parser, template->tokens);
   assert(ok == TGSI_PARSE_OK);

   struct tegra_vpe_shader vpe;
   tegra_tgsi_to_vpe(&vpe, &parser);

   int num_instructions = list_length(&vpe.instructions);
   assert(num_instructions < 256);
   int num_commands = 2 + num_instructions * 4;
   uint32_t *commands = MALLOC(num_commands * sizeof(uint32_t));
   if (!commands) {
      FREE(so);
      return NULL;
   }

   commands[0] = host1x_opcode_imm(TGR3D_VP_UPLOAD_INST_ID, 0);
   commands[1] = host1x_opcode_nonincr(TGR3D_VP_UPLOAD_INST,
                                       num_instructions * 4);

   struct vpe_instr *last = list_last_entry(&vpe.instructions, struct vpe_instr, link);
   int offset = 2;
   list_for_each_entry(struct vpe_instr, instr, &vpe.instructions, link) {
      bool end_of_program = instr == last;
      tegra_vpe_pack(commands + offset, instr, end_of_program);
      offset += 4;
   }

   so->blob.commands = commands;
   so->blob.num_commands = num_commands;
   so->output_mask = vpe.output_mask;

   return so;
}

static void
tegra_bind_vs_state(struct pipe_context *pcontext, void *so)
{
   tegra_context(pcontext)->vshader = so;
}

static void
tegra_delete_vs_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

static void *
tegra_create_fs_state(struct pipe_context *pcontext,
                      const struct pipe_shader_state *template)
{
   struct tegra_fragment_shader_state *so =
      CALLOC_STRUCT(tegra_fragment_shader_state);

   if (!so)
      return NULL;

   so->base = *template;

   if (tegra_debug & TEGRA_DEBUG_TGSI) {
      fprintf(stderr, "DEBUG: TGSI:\n");
      tgsi_dump(template->tokens, 0);
      fprintf(stderr, "\n");
   }

   struct tgsi_parse_context parser;
   unsigned ok = tgsi_parse_init(&parser, template->tokens);
   assert(ok == TGSI_PARSE_OK);

   struct tegra_fp_shader fp;
   tegra_tgsi_to_fp(&fp, &parser);

   struct util_dynarray buf;
   util_dynarray_init(&buf, NULL);

#define PUSH(x) util_dynarray_append(&buf, uint32_t, (x))
   PUSH(host1x_opcode_incr(TGR3D_ALU_BUFFER_SIZE, 1));
   PUSH(0x58000000);

   PUSH(host1x_opcode_imm(TGR3D_FP_PSEQ_QUAD_ID, 0));
   PUSH(host1x_opcode_imm(TGR3D_FP_UPLOAD_INST_ID_COMMON, 0));
   PUSH(host1x_opcode_imm(TGR3D_FP_UPLOAD_MFU_INST_ID, 0));
   PUSH(host1x_opcode_imm(TGR3D_FP_UPLOAD_ALU_INST_ID, 0));

   int num_fp_instrs = list_length(&fp.fp_instructions);
   assert(num_fp_instrs < 64);

   PUSH(host1x_opcode_incr(TGR3D_FP_PSEQ_ENGINE_INST, 1));
   PUSH(0x20006000 | num_fp_instrs);

   PUSH(host1x_opcode_incr(TGR3D_FP_PSEQ_DW_CFG, 1));
   PUSH(0x00000040);

   PUSH(host1x_opcode_imm(TGR3D_FP_PSEQ_UPLOAD_INST_BUFFER_FLUSH, 0));

   PUSH(host1x_opcode_nonincr(TGR3D_FP_PSEQ_UPLOAD_INST, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(0x00000000);

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_MFU_SCHED, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(tegra_fp_pack_sched(&instr->mfu_sched));

   int num_mfu_instrs = list_length(&fp.mfu_instructions);
   assert(num_mfu_instrs < 64); // TODO: not sure if this is really correct

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_MFU_INST, num_mfu_instrs * 2));
   list_for_each_entry(struct fp_mfu_instr, instr, &fp.mfu_instructions, link) {
      uint32_t words[2];
      tegra_fp_pack_mfu(words, instr);
      PUSH(words[0]);
      PUSH(words[1]);
   }

   // TODO: emit actual instructions here
   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_TEX_INST, num_fp_instrs));
   for (int i = 0; i < num_fp_instrs; ++i)
      PUSH(0x00000000);

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_SCHED, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(tegra_fp_pack_sched(&instr->alu_sched));

   int num_alu_instrs = list_length(&fp.alu_instructions);
   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_INST,
        num_alu_instrs * 4 * 2));
   list_for_each_entry(struct fp_alu_instr_packet, instr, &fp.alu_instructions, link) {
      for (int i = 0; i < 4; ++i) {
         uint32_t words[2];
         tegra_fp_pack_alu(words, instr->slots + i);
         PUSH(words[0]);
         PUSH(words[1]);
      }
   }

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_INST_COMPLEMENT, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(0x00000000);

   PUSH(host1x_opcode_nonincr(TGR3D_FP_UPLOAD_DW_INST, num_fp_instrs));
   list_for_each_entry(struct fp_instr, instr, &fp.fp_instructions, link)
      PUSH(tegra_fp_pack_dw(&instr->dw));

   uint32_t tram_setup = 0;
   tram_setup |= TGR3D_VAL(TRAM_SETUP, USED_TRAM_ROWS_NB, fp.info.max_tram_row);
   tram_setup |= TGR3D_VAL(TRAM_SETUP, DIV64, 64 / fp.info.max_tram_row);

   PUSH(host1x_opcode_incr(TGR3D_TRAM_SETUP, 1));
   PUSH(tram_setup);

#undef PUSH
   util_dynarray_trim(&buf);

   so->blob.num_commands = buf.size / sizeof(uint32_t);
   so->blob.commands = buf.data;
   so->info = fp.info;
   return so;
}

static void
tegra_bind_fs_state(struct pipe_context *pcontext, void *so)
{
   tegra_context(pcontext)->fshader = so;
}

static void
tegra_delete_fs_state(struct pipe_context *pcontext, void *so)
{
   FREE(so);
}

void
tegra_context_program_init(struct pipe_context *pcontext)
{
   pcontext->create_vs_state = tegra_create_vs_state;
   pcontext->bind_vs_state = tegra_bind_vs_state;
   pcontext->delete_vs_state = tegra_delete_vs_state;

   pcontext->create_fs_state = tegra_create_fs_state;
   pcontext->bind_fs_state = tegra_bind_fs_state;
   pcontext->delete_fs_state = tegra_delete_fs_state;
}
