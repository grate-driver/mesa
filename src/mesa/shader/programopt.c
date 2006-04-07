
/**
 * \file  Vertex/Fragment program optimizations and transformations.
 */



/**
 * This is used for helping with position-invariant vertex programs.
 * It appends extra instructions to the given program to do the
 * vertex position transformation (multiply by the MVP matrix).
 */
void
_mesa_append_modelview_code(GLcontext *ctx, struct vertex_program *program)
{
   struct vp_instruction newInst[5], *newProgram;
   GLuint i, origLen;

   /*
    * Setup state references for the modelview/projection matrix.
    * XXX we should check if these state vars are already declared.
    */
   static const GLint mvpState[4][5] = {
      { STATE_MATRIX, STATE_MVP, 0, 0, 0 },  /* state.matrix.mvp.row[0] */
      { STATE_MATRIX, STATE_MVP, 0, 1, 1 },  /* state.matrix.mvp.row[1] */
      { STATE_MATRIX, STATE_MVP, 0, 2, 2 },  /* state.matrix.mvp.row[2] */
      { STATE_MATRIX, STATE_MVP, 0, 3, 3 },  /* state.matrix.mvp.row[3] */
   };
   GLint row[4];

   for (i = 0; i < 4; i++) {
      row[i] = _mesa_add_state_reference(program->Parameters, mvpState[i]);
   }

   /*
    * Generate instructions:
    * DP4 result.position.x, mvp.row[0], vertex.position;
    * DP4 result.position.y, mvp.row[1], vertex.position;
    * DP4 result.position.z, mvp.row[2], vertex.position;
    * DP4 result.position.w, mvp.row[3], vertex.position;
    */
   for (i = 0; i < 4; i++) {
      _mesa_init_vp_instruction(newInst + i);
      newInst[i].Opcode = VP_OPCODE_DP4;
      newInst[i].DstReg.File = PROGRAM_OUTPUT;
      newInst[i].DstReg.Index = VERT_RESULT_HPOS;
      newInst[i].DstReg.WriteMask = (WRITEMASK_X << i);
      newInst[i].SrcReg[0].File = PROGRAM_STATE_VAR;
      newInst[i].SrcReg[0].Index = row[i];
      newInst[i].SrcReg[0].Swizzle = SWIZZLE_NOOP;
      newInst[i].SrcReg[1].File = PROGRAM_INPUT;
      newInst[i].SrcReg[1].Index = VERT_ATTRIB_POS;
      newInst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
   }
   newInst[4].Opcode = VP_OPCODE_END;

   /*
    * Append new instructions onto program.
    */
   origLen = program->Base.NumInstructions;

   newProgram = (struct vp_instruction *)
      _mesa_realloc(program->Instructions,
                    origLen * sizeof(struct vp_instruction),
                    (origLen + 4) * sizeof(struct vp_instruction));
   if (!newProgram) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(generating position transformation code)");
      return;
   }

   /* subtract one to overwrite original END instruction */
   _mesa_memcpy(newProgram + origLen - 1, newInst, sizeof(newInst));

   program->Instructions = newProgram;
   program->Base.NumInstructions = origLen + 4;
   program->InputsRead |= VERT_BIT_POS;
   program->OutputsWritten |= (1 << VERT_RESULT_HPOS);
}



/**
 * Append extra instructions onto the given fragment program to implement
 * the fog mode specified by program->FogOption.
 */
void
_mesa_append_fog_code(GLcontext *ctx, struct fragment_program *program)
{
   struct fp_instruction newInst[10];


   switch (program->FogOption) {
   case GL_LINEAR:
      /* lerp */
      break;
   case GL_EXP:
      break;
   case GL_EXP2:
      break;
   case GL_NONE:
      /* no-op */
      return;
   default:
      _mesa_problem(ctx, "Invalid fog option");
      return;
   }




}
