# Archived TXP investigation: not an enabled driver feature

These unapplied patches preserve the R7–R10 experiments for reproducibility.
They are **unfinished**, are not applied by any build or launch script, and
are not part of the validated R11 showcase driver. Do not treat them as a
working TXP implementation.

The series starts at Mesa lab commit
`8ecd33d3284f6a0bba10346231992099409222ed`; apply the numbered patches in order
only in a separate research checkout if investigating the same failure.
The patches reproduce the archived R10 source, including known defects.

- R7 added a restricted MFU reciprocal/multiply path for projective texture
  coordinates and TEMP0 coordinate tracking. CPU checks and software controls
  passed, but the first projective GPU test retained only the clear color.
- R8 computed the actual DW execution-block index instead of always using
  block 1. That correction alone did not fix the blank projective result.
- R9 paired the ALU buffer and recirculation settings used by OpenTegra.
  Varying-coordinate TXP and fixed-function GL_REPLACE each passed 12,544
  framebuffer samples. Constant-coordinate TXP drew the wrong quadrant.
- R10 fixed the mismatch between MESA_SHADER_FRAGMENT and the older Gallium
  PIPE_SHADER_FRAGMENT enum. Direct uniform colors then passed, but constant
  TXP still sampled the wrong quadrant. The independent enum fix is already
  included in the active validated branch.

A further review found that `simple_coord_mask` was not initialized in
`grate_tgsi_to_fp`; its reset had been inserted into the MUL helper instead.
That must be fixed and tested with poisoned initial state before using this
research in a future driver. No fix is silently folded into this archive.
The validated R11 source starts from the public branch without this TXP path
and therefore does not contain that field or defect.

Every failed GPU series stopped before the next scheduled case. Kernel and
protected-file checks remained clean; there was no system Mesa replacement.
The original full logs and immutable private payloads remain local.

References: GRATE's fragment ISA documentation and OpenTegra's fragment
sequencer setup informed the experiments:

- https://github.com/grate-driver/grate/blob/master/docs/fragment-shader-isa.md
- https://github.com/grate-driver/xf86-video-opentegra/blob/master/src/gpu/gr3d.c
