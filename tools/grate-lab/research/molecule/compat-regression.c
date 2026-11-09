#include "grate_program.c"
#include "tgsi/tgsi_text.h"
#include <math.h>
static uint32_t captured_uniforms[64];
static unsigned num_captured;
static int test_stream_push(struct grate_stream *stream, uint32_t word) {
  assert(num_captured < 64);
  captured_uniforms[num_captured++] = word;
  return 0;
}
#define grate_stream_push test_stream_push
#include "grate_state.c"
#undef grate_stream_push
static void check_uniform_upload(void) {
  struct grate_context ctx = {0};
  struct grate_channel channel = {0};
  ctx.gr3d = &channel;
  float values[] = {0.25f, 0.75f, 0.0f, 1.0f}, vertex[] = {9, 8, 7, 6};
  struct pipe_constant_buffer fragment = {.user_buffer = values,
                                          .buffer_size = sizeof(values)};
  struct pipe_constant_buffer vert = {.user_buffer = vertex,
                                      .buffer_size = sizeof(vertex)};
  grate_set_constant_buffer(&ctx.base, PIPE_SHADER_VERTEX, 0, false, &vert);
  grate_set_constant_buffer(&ctx.base, PIPE_SHADER_FRAGMENT, 0, false,
                            &fragment);
  emit_fs_uniforms(&ctx);
  assert(num_captured == 5);
  assert(captured_uniforms[0] == host1x_opcode_incr(TGR3D_FP_CONST(0), 4));
  assert(captured_uniforms[1] == 0x3a000 && captured_uniforms[2] == 0x3d000 &&
         captured_uniforms[3] == 0 && captured_uniforms[4] == 0x3e000);
  puts("PASS fragment PIPE-stage selection and exact FP20 upload (vertex "
       "sentinel isolated)");
}
static float value(struct fp_alu_src_operand s, float *r, float *u) {
  assert(!s.negate && !s.absolute_value && !s.scale_by_two && !s.minus_one);
  if (s.index == 31) {
    assert(s.datatype == FP_DATATYPE_FIXED10);
    return s.sub_reg_select_high ? 1 : 0;
  }
  assert(s.datatype == FP_DATATYPE_FP20);
  if (s.index >= 32 && s.index < 64)
    return u[s.index - 32];
  assert(s.index < 16);
  return r[s.index];
}
static unsigned trials;
static void check(const char *text, int project, int constant, int qchan) {
  struct tgsi_token *t = calloc(4096, sizeof(*t));
  assert(tgsi_text_translate(text, t, 4096));
  struct tgsi_parse_context parser;
  assert(tgsi_parse_init(&parser, t) == TGSI_PARSE_OK);
  struct grate_fp_shader fp;
  memset(&fp, 0xa5, sizeof(fp));
  grate_tgsi_to_fp(&fp, &parser);
  assert(fp.info.unverified_uniform_texcoords == (constant != 0));
  struct fp_mfu_instr *mfus[64];
  struct fp_alu_instr_packet *alus[64];
  unsigned nm = 0, na = 0;
  list_for_each_entry(struct fp_mfu_instr, m, &fp.mfu_instructions, link) {
    assert(nm < 64);
    mfus[nm++] = m;
    uint32_t words[2];
    grate_fp_pack_mfu(words, m);
    uint64_t packed = ((uint64_t)words[0] << 32) | words[1];
    assert((packed >> 58) == m->sfu.reg);
    assert(((packed >> 54) & 15) == m->sfu.op);
    assert(((packed >> 32) & 15) == m->mul[0].src[0]);
    assert(((packed >> 36) & 15) == m->mul[0].src[1]);
    assert(((packed >> 40) & 7) == m->mul[0].dst);
    assert(((packed >> 43) & 15) == m->mul[1].src[0]);
    assert(((packed >> 47) & 15) == m->mul[1].src[1]);
    assert(((packed >> 51) & 7) == m->mul[1].dst);
    for (unsigned j = 0; j < 4; j++) {
      assert(((packed >> (j * 7 + 1)) & 3) == m->var[j].op);
      assert(((packed >> (j * 7 + 3)) & 15) == m->var[j].tram_row);
    }
  }
  list_for_each_entry(struct fp_alu_instr_packet, a, &fp.alu_instructions,
                      link) {
    assert(na < 64);
    alus[na++] = a;
  }
  for (unsigned test = 0; test < 512; test++) {
    float q = (test % 31 + 1) * 0.25f;
    if (test & 32)
      q = -q;
    float input[4] = {((int)(test % 73) - 36) * 0.03125f,
                      ((int)(test % 47) - 23) * 0.0625f, 0.25f, q};
    input[qchan] = q;
    float uniform[32] = {input[0], input[1], input[2], input[3]};
    float r[16] = {0};
    r[4] = 1;
    unsigned tex = 0;
    list_for_each_entry(struct fp_instr, i, &fp.fp_instructions, link) {
      assert(i->mfu_sched.address + i->mfu_sched.num_instructions <= nm);
      for (unsigned n = 0; n < i->mfu_sched.num_instructions; n++) {
        struct fp_mfu_instr *m = mfus[i->mfu_sched.address + n];
        float sfu = m->sfu.op == FP_SFU_OP_RCP ? 1.0f / r[m->sfu.reg] : 0;
        float products[2] = {0};
        for (unsigned j = 0; j < 2; j++)
          if (m->mul[j].dst >= 4) {
            float v[2];
            for (unsigned k = 0; k < 2; k++) {
              unsigned src = m->mul[j].src[k];
              assert(src < 4 || src == 10);
              v[k] = src == 10 ? sfu : r[src];
            }
            products[j] = v[0] * v[1];
          }
        for (unsigned j = 0; j < 2; j++)
          if (m->mul[j].dst >= 4)
            r[m->mul[j].dst - 4] = products[j];
        for (unsigned j = 0; j < 4; j++)
          if (m->var[j].op == FP_VAR_OP_FP20)
            r[j] = input[j];
      }
      if (i->tex_word) {
        assert(i->tex_word == 0x400);
        tex++;
        float expected0 = input[0] / (project ? q : 1),
              expected1 = input[1] / (project ? q : 1);
        assert(fabsf(r[0] - expected0) < 0.000001f);
        assert(fabsf(r[1] - expected1) < 0.000001f);
      }
      assert(i->alu_sched.address + i->alu_sched.num_instructions <= na);
      for (unsigned n = 0; n < i->alu_sched.num_instructions; n++) {
        struct fp_alu_instr_packet *a = alus[i->alu_sched.address + n];
        for (unsigned j = 0; j < 4; j++) {
          struct fp_alu_instr *op = &a->slots[j];
          assert(op->op == FP_ALU_OP_MAD);
          if (op->dst.index == 31)
            continue;
          assert(op->dst.index < 4 && !op->dst.write_low_sub_reg &&
                 !op->dst.write_high_sub_reg);
          r[op->dst.index] =
              value(op->src[0], r, uniform) * value(op->src[1], r, uniform) +
              value(op->src[2], r, uniform) * value(op->src[3], r, uniform);
        }
      }
    }
    assert(tex == 1);
    trials++;
  }
  struct pipe_shader_state state = {.type = PIPE_SHADER_IR_TGSI, .tokens = t};
  struct grate_context context = {0};
  struct grate_fragment_shader_state *fs =
      grate_create_fs_state(&context.base, &state);
  assert(fs);
  assert(fs->info.num_inputs == (constant ? 0 : 1));
  unsigned dw_config = 0, found = 0, expected_exec = 0, exec = 0;
  list_for_each_entry(struct fp_instr, i, &fp.fp_instructions, link) {
    exec++;
    if (i->dw.enable)
      expected_exec = exec;
  }
  for (int j = 0; j < fs->blob.num_commands - 1; j++)
    if (fs->blob.commands[j] == host1x_opcode_incr(TGR3D_FP_PSEQ_DW_CFG, 1)) {
      dw_config = fs->blob.commands[j + 1];
      found++;
    }
  assert(found == 1 && dw_config == (expected_exec << 6));
  for (unsigned mode = 0; mode < 3; mode++) {
    struct grate_shader_blob *b = &fs->blend_blob[mode];
    unsigned n = 0;
    for (int j = 0; j < b->num_commands - 1; j++)
      if (b->commands[j] == host1x_opcode_incr(TGR3D_FP_PSEQ_DW_CFG, 1)) {
        assert(b->commands[j + 1] == ((expected_exec + 2) << 6));
        n++;
      }
    assert(n == 1);
    unsigned upload = 0, flush = 0;
    for (int j = 0; j < b->num_commands; j++) {
      if ((b->commands[j] >> 16) ==
          (host1x_opcode_nonincr(TGR3D_FP_PSEQ_UPLOAD_INST, 0) >> 16))
        upload = j;
      if (b->commands[j] ==
          host1x_opcode_imm(TGR3D_FP_PSEQ_UPLOAD_INST_BUFFER_FLUSH, 0))
        flush = j;
    }
    assert(upload && flush == upload + 1 + expected_exec + 2);
  }
  unsigned buffer_found = 0, recirc_found = 0;
  for (int j = 0; j < fs->blob.num_commands - 1; j++) {
    if (fs->blob.commands[j] == host1x_opcode_incr(TGR3D_ALU_BUFFER_SIZE, 1)) {
      assert(fs->blob.commands[j + 1] == (exec > 1 ? 0x4a000000 : 0x58000000));
      buffer_found++;
    }
    if (fs->blob.commands[j] == host1x_opcode_incr(0x501, 1)) {
      assert(fs->blob.commands[j + 1] == 0x223212cf);
      recirc_found++;
    }
  }
  assert(buffer_found == 1 && recirc_found == (exec > 1 ? 1 : 0));
  tgsi_parse_free(&parser);
  free(t);
}
static void native_shader_compile(void) {
  const char *source =
      "FRAG\nDCL OUT[0], COLOR\nDCL SAMP[0]\nDCL SVIEW[0], 2D, FLOAT\nDCL "
      "CONST[0][0..1]\nDCL TEMP[0]\n0: MOV TEMP[0], CONST[0][1].xyxw\n1: TXP "
      "TEMP[0], TEMP[0], SAMP[0], 2D\n2: MUL OUT[0], TEMP[0], CONST[0][0]\n3: "
      "END\n";
  struct tgsi_token *t = calloc(4096, sizeof(*t));
  assert(tgsi_text_translate(source, t, 4096));
  struct pipe_shader_state state = {.type = PIPE_SHADER_IR_TGSI, .tokens = t};
  struct grate_context context = {0};
  struct grate_fragment_shader_state *fs =
      grate_create_fs_state(&context.base, &state);
  assert(fs && fs->info.unverified_uniform_texcoords);
  unsigned tex = 0, alu = 0, dw = 0;
  for (int i = 0; i < fs->blob.num_commands - 2; i++) {
    if (fs->blob.commands[i] ==
        host1x_opcode_nonincr(TGR3D_FP_UPLOAD_TEX_INST, 2)) {
      assert(fs->blob.commands[i + 1] == 0 &&
             fs->blob.commands[i + 2] == 0x400);
      tex++;
    }
    if (fs->blob.commands[i] ==
        host1x_opcode_nonincr(TGR3D_FP_UPLOAD_ALU_SCHED, 2)) {
      assert(fs->blob.commands[i + 1] == 1 && fs->blob.commands[i + 2] == 5);
      alu++;
    }
    if (fs->blob.commands[i] == host1x_opcode_incr(TGR3D_FP_PSEQ_DW_CFG, 1)) {
      assert(fs->blob.commands[i + 1] == 128);
      dw++;
    }
  }
  assert(tex == 1 && alu == 1 && dw == 1);
  puts("PASS observed native Molecule shader compiles with valid schedules and "
       "remains guarded for unverified uniform coordinates");
  free(t);
}

static float blend_value(struct fp_alu_src_operand s, float r[4][2]) {
  float x = s.index == 31 ? (s.sub_reg_select_high ? 1 : 0)
                          : r[s.index][s.sub_reg_select_high];
  if (s.minus_one)
    x -= 1;
  if (s.negate)
    x = -x;
  return x;
}
static void check_blend(void) {
  unsigned total = 0;
  for (unsigned srcbase = 0; srcbase <= 2; srcbase += 2)
    for (unsigned mode = 1; mode <= 3; mode++) {
      struct grate_fp_shader fp = {0};
      list_inithead(&fp.fp_instructions);
      list_inithead(&fp.alu_instructions);
      list_inithead(&fp.mfu_instructions);
      struct fp_instr original = {
          .dw = {.enable = true,
                 .index = 1,
                 .src_regs = srcbase ? FP_DW_REGS_R2_R3 : FP_DW_REGS_R0_R1}};
      list_addtail(&original.link, &fp.fp_instructions);
      append_source_over(&fp, mode);
      assert(!original.dw.enable && list_length(&fp.fp_instructions) == 3);
      struct fp_alu_instr_packet *packets[2];
      unsigned n = 0;
      list_for_each_entry(struct fp_alu_instr_packet, p, &fp.alu_instructions,
                          link) {
        assert(n < 2);
        packets[n++] = p;
      }
      assert(n == 2);
      struct fp_instr *last =
          list_last_entry(&fp.fp_instructions, struct fp_instr, link);
      assert(last->pseq_word == 0x00810008 && last->dw.enable &&
             last->alu_sched.address == 1);
      for (unsigned j = 0; j < 513; j++) {
        float a = (j % 33) / 32.f;
        float source[4] = {((j * 7) % 31) / 30.f, ((j * 11) % 23) / 22.f,
                           ((j * 13) % 19) / 18.f, a};
        float dest[4] = {0.23f, 0.61f, 0.87f, (j % 17) / 16.f};
        if (mode == 1)
          for (unsigned c = 0; c < 3; c++)
            source[c] *= a;
        float regs[4][2] = {{0}};
        for (unsigned c = 0; c < 4; c++)
          regs[srcbase + c / 2][c & 1] = source[c];
        for (unsigned stage = 0; stage < 2; stage++) {
          if (stage)
            for (unsigned c = 0; c < 4; c++)
              regs[c / 2][c & 1] = dest[c];
          float old[4][2];
          memcpy(old, regs, sizeof(old));
          for (unsigned c = 0; c < 4; c++) {
            struct fp_alu_instr *op = &packets[stage]->slots[c];
            uint32_t w[2];
            grate_fp_pack_alu(w, op);
            assert(((w[1] >> 2) & 1) == (stage == 1 && mode == 2 && c == 3));
            float v =
                blend_value(op->src[0], old) * blend_value(op->src[1], old) +
                blend_value(op->src[2], old) *
                    (op->multiply_addend ? blend_value(op->src[3], old) : 1.f);
            regs[op->dst.index][op->dst.write_high_sub_reg] =
                fminf(1, fmaxf(0, v));
          }
        }
        for (unsigned c = 0; c < 4; c++) {
          float factor = c == 3 ? (mode == 2 ? a : 1.f) : (mode == 1 ? 1.f : a);
          float expected = source[c] * factor + dest[c] * (1 - a);
          assert(fabsf(regs[2 + c / 2][c & 1] - expected) < 0.000001f);
          total++;
        }
      }
    }
  printf("PASS source-over IR and packed D-enable: %u component comparisons "
         "across both output register banks and three blend modes\n",
         total);
}

int main(void) {
  check_blend();
  native_shader_compile();
  check_uniform_upload();
  const char *decl = "FRAG\nDCL IN[0], GENERIC[0], PERSPECTIVE\nDCL OUT[0], "
                     "COLOR\nDCL SAMP[0]\nDCL SVIEW[0], 2D, FLOAT\n";
  char s[4096];
  snprintf(s, sizeof(s), "%s0: TEX OUT[0], IN[0], SAMP[0], 2D\n1: END\n", decl);
  check(s, 0, 0, 3);
  snprintf(s, sizeof(s), "%s0: TXP OUT[0], IN[0], SAMP[0], 2D\n1: END\n", decl);
  check(s, 1, 0, 3);
  snprintf(s, sizeof(s), "%s0: TXP OUT[0], IN[0].xyzz, SAMP[0], 2D\n1: END\n",
           decl);
  check(s, 1, 0, 2);
  snprintf(s, sizeof(s),
           "%sDCL TEMP[0]\n0: MOV TEMP[0], IN[0].xyxw\n1: TXP OUT[0], TEMP[0], "
           "SAMP[0], 2D\n2: END\n",
           decl);
  check(s, 1, 0, 3);
  check("FRAG\nDCL OUT[0], COLOR\nDCL SAMP[0]\nDCL SVIEW[0], 2D, FLOAT\nDCL "
        "CONST[0][0]\nDCL TEMP[0]\n0: MOV TEMP[0], CONST[0][0].xyxw\n1: TXP "
        "OUT[0], TEMP[0], SAMP[0], 2D\n2: END\n",
        1, 1, 3);
  printf("PASS TEX/TXP input/constant coordinates: %u finite trials, MFU "
         "encoding and schedule bounds\n",
         trials);
  return 0;
}
