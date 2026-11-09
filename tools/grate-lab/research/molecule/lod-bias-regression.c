static int previous_main(void);
#define main previous_main
#include "compat-regression.c"
#undef main
static struct grate_fragment_shader_state *make_bias_shader(int conflict,
                                                            int project) {
  char s[2048];
  snprintf(s, sizeof(s),
           "FRAG\nDCL IN[0], GENERIC[0], PERSPECTIVE\nDCL OUT[0], COLOR\nDCL "
           "SAMP[0]\nDCL SVIEW[0], 2D, FLOAT\n%s%s",
           conflict ? "DCL CONST[0][0..7]\nDCL TEMP[0]\n" : "",
           conflict  ? "0: TEX TEMP[0], IN[0], SAMP[0], 2D\n1: MUL OUT[0], "
                       "TEMP[0], CONST[0][7]\n2: END\n"
           : project ? "0: TXP OUT[0], IN[0], SAMP[0], 2D\n1: END\n"
                     : "0: TEX OUT[0], IN[0], SAMP[0], 2D\n1: END\n");
  struct tgsi_token *t = calloc(4096, sizeof(*t));
  assert(tgsi_text_translate(s, t, 4096));
  struct pipe_shader_state state = {.type = PIPE_SHADER_IR_TGSI, .tokens = t};
  struct grate_context ctx = {0};
  struct grate_fragment_shader_state *fs =
      grate_create_fs_state(&ctx.base, &state);
  assert(fs);
  free(t);
  return fs;
}
static void check_bias(void) {
  unsigned checks = 0;
  for (unsigned project = 0; project < 2; project++) {
    const char *text =
        project ? "FRAG\nDCL IN[0], GENERIC[0], PERSPECTIVE\nDCL OUT[0], "
                  "COLOR\nDCL SAMP[0]\nDCL SVIEW[0], 2D, FLOAT\n0: TXP OUT[0], "
                  "IN[0], SAMP[0], 2D\n1: END\n"
                : "FRAG\nDCL IN[0], GENERIC[0], PERSPECTIVE\nDCL OUT[0], "
                  "COLOR\nDCL SAMP[0]\nDCL SVIEW[0], 2D, FLOAT\n0: TEX OUT[0], "
                  "IN[0], SAMP[0], 2D\n1: END\n";
    struct tgsi_token t[256];
    assert(tgsi_text_translate(text, t, 256));
    struct tgsi_parse_context parser;
    assert(tgsi_parse_init(&parser, t) == TGSI_PARSE_OK);
    struct grate_fp_shader fp;
    grate_tgsi_to_fp(&fp, &parser);
    struct fp_instr *tex =
        list_first_entry(&fp.fp_instructions, struct fp_instr, link);
    struct fp_sched mfu = tex->mfu_sched;
    append_sampler_bias(&fp);
    assert(list_length(&fp.fp_instructions) == 3 && tex->tex_word == 0x1400 &&
           tex->mfu_sched.num_instructions == 0);
    struct fp_instr *coord =
        list_first_entry(&fp.fp_instructions, struct fp_instr, link);
    assert(coord->mfu_sched.address == mfu.address &&
           coord->mfu_sched.num_instructions == mfu.num_instructions);
    struct fp_instr *load = list_entry(coord->link.next, struct fp_instr, link);
    assert(load->link.next == &tex->link &&
           load->alu_sched.num_instructions == 1);
    struct fp_alu_instr_packet *alu =
        list_last_entry(&fp.alu_instructions, struct fp_alu_instr_packet, link);
    struct fp_alu_instr *op = &alu->slots[0];
    assert(op->dst.index == 3 && op->dst.write_low_sub_reg &&
           op->dst.write_high_sub_reg && op->src[0].index == 63 &&
           op->src[0].datatype == FP_DATATYPE_FP20);
    uint32_t packed[2];
    grate_fp_pack_alu(packed, op);
    assert(((packed[0] >> 6) & 127) == 63);
    assert((packed[0] & (3u << 13)) == (3u << 13)); /* rA selector bits38..44 */
    for (int j = -64; j <= 64; j++) {
      float rows[16] = {.125f, .875f, .5f, 2.f};
      float uniforms[32] = {0};
      uniforms[31] = j * .25f;
      rows[op->dst.index] = value(op->src[0], rows, uniforms) *
                                value(op->src[1], rows, uniforms) +
                            value(op->src[2], rows, uniforms);
      assert(rows[0] == .125f && rows[1] == .875f && rows[2] == .5f &&
             rows[3] == j * .25f);
      checks++;
    }
    struct grate_fragment_shader_state *fs = make_bias_shader(0, project);
    assert(fs->uses_texture && !fs->bias_uniform_conflict);
    for (unsigned mode = 0; mode < 4; mode++) {
      struct grate_shader_blob *b = &fs->biased_blob[mode];
      assert(b->commands);
      unsigned dw = 0, txb = 0;
      for (int j = 0; j < b->num_commands - 1; j++) {
        if (b->commands[j] == host1x_opcode_incr(TGR3D_FP_PSEQ_DW_CFG, 1)) {
          assert(b->commands[j + 1] == ((mode ? 5 : 3) << 6));
          dw++;
        }
        if (b->commands[j] ==
            host1x_opcode_nonincr(TGR3D_FP_UPLOAD_TEX_INST, mode ? 5 : 3)) {
          unsigned n = mode ? 5 : 3;
          for (unsigned k = 1; k <= n; k++)
            if (b->commands[j + k] == 0x1400)
              txb++;
        }
      }
      assert(dw == 1 && txb == 1);
    }
    struct grate_context ctx = {0};
    struct grate_channel ch = {0};
    struct pipe_sampler_state sampler = {.lod_bias = .25f};
    ctx.gr3d = &ch;
    ctx.fshader = fs;
    ctx.fragment_sampler = &sampler;
    float user[32];
    for (unsigned k = 0; k < 32; k++)
      user[k] = k;
    ctx.constant_buffer[PIPE_SHADER_FRAGMENT] = (struct pipe_constant_buffer){
        .user_buffer = user, .buffer_size = sizeof(user)};
    for (unsigned mode = 0; mode < 4; mode++) {
      ctx.blend_mode = mode;
      assert(select_sampler_bias(&ctx) && ctx.sampler_bias_active);
      num_captured = 0;
      emit_fs_uniforms(&ctx);
      assert(num_captured == 35 &&
             captured_uniforms[33] ==
                 host1x_opcode_incr(TGR3D_FP_CONST(31), 1) &&
             captured_uniforms[34] == 0x3a000);
      assert(captured_uniforms[32] == grate_float_to_fp20(31.f));
    }
    sampler.lod_bias = -.25f;
    assert(select_sampler_bias(&ctx));
    num_captured = 0;
    emit_fs_uniforms(&ctx);
    assert(captured_uniforms[34] == 0xba000);
    sampler.lod_bias = 0;
    assert(select_sampler_bias(&ctx) && !ctx.sampler_bias_active);
    num_captured = 0;
    emit_fs_uniforms(&ctx);
    assert(num_captured == 33 &&
           captured_uniforms[32] == grate_float_to_fp20(31.f));
    ctx.constant_buffer[PIPE_SHADER_FRAGMENT] =
        (struct pipe_constant_buffer){0};
    sampler.lod_bias = .25f;
    assert(select_sampler_bias(&ctx));
    num_captured = 0;
    emit_fs_uniforms(&ctx);
    assert(num_captured == 2 && captured_uniforms[1] == 0x3a000);
    sampler.lod_bias = NAN;
    assert(!select_sampler_bias(&ctx));
    sampler.lod_bias = INFINITY;
    assert(!select_sampler_bias(&ctx));
    sampler.lod_bias = 17;
    assert(!select_sampler_bias(&ctx));
    sampler.lod_bias = -16;
    assert(select_sampler_bias(&ctx));
    sampler.lod_bias = 16;
    assert(select_sampler_bias(&ctx));
    grate_delete_fs_state(&ctx.base, fs);
    tgsi_parse_free(&parser);
  }
  struct grate_fragment_shader_state *collision = make_bias_shader(1, 0);
  assert(collision->bias_uniform_conflict && collision->uses_texture);
  for (unsigned k = 0; k < 4; k++)
    assert(!collision->biased_blob[k].commands);
  struct grate_context ctx = {.fshader = collision};
  struct pipe_sampler_state sampler = {.lod_bias = .25f};
  ctx.fragment_sampler = &sampler;
  assert(!select_sampler_bias(&ctx) && !ctx.sampler_bias_active);
  sampler.lod_bias = 0;
  assert(select_sampler_bias(&ctx) && !ctx.sampler_bias_active);
  grate_delete_fs_state(&ctx.base, collision);
  printf("PASS LOD bias: %u UV-preserving FP20 loads, TEX/TXP stage split, "
         "eight shader variants, exact signed upload, zero-bias restoration "
         "and uniform collision/range guards\n",
         checks);
}
int main(void) {
  int result = previous_main();
  assert(result == 0);
  check_bias();
  return 0;
}
