# Native Molecule compatibility investigation

These are **unapplied research snapshots**, separate from the validated R11
showcase driver. None is installed by the main build scripts. Full Molecule
compatibility and complete OpenGL support have not been achieved.

The chosen independent application is the unmodified, signed Alpine ARMv7
XScreenSaver 6.15 `molecule` executable. The private test loads a three-atom
water PDB at 640x480 with labels, titles and electron shells disabled. A
diagnostic preload reports the real EGL renderer, counts swaps and reads
back frame 3; it does not suppress application features or GL calls.
Softpipe renders the actual red oxygen and white hydrogen atoms. Hardware
has not yet reached a successful frame in these snapshots.

## Snapshot reproduction

Start from lab commit `982f59a27b958f08301b5973dd91b4c2404b107e` in a separate
checkout. Each `rNN-from-982f59a.patch` is a **complete cumulative patch**
against that commit. Choose one; do not apply them as a series. Build and
stage only in an isolated prefix, following the lab README. Do not replace
system Mesa, Xorg, kernel or boot configuration.

- R12 initializes all fragment metadata, permits native MOV/TXP/MUL shader
  compilation, and ports the varying-coordinate TXP path. The known-wrong
  uniform-coordinate path is tagged and aborts before actual draw execution.
  CPU compilation is not a claim that uniform TXP is correct. Molecule next
  aborts at blend binding.
- R13 defers unsupported blend validation from binding to actual execution.
  Molecule really requires additive SRC_ALPHA / INV_SRC_ALPHA for both RGB
  and alpha; the draw is rejected before submission.
- R14 appends restricted source-over variants: premultiplied, straight alpha,
  and straight RGB with separate ONE alpha. It normalizes TEX's r0/r1 result
  before destination fetch, preserves original alpha, and enables the ALU D
  multiplier for straight-alpha output alpha. Actual DW block is recomputed.
  CPU math/encoding passes, but GPU readback has spatially wrong pixels.
- R15 moves the PSEQ upload-buffer commit after upload, matching OpenTegra.
  This alone does not resolve corruption, which also affects premultiplied
  draws. The failure values match the previous case's destination instead of
  the newly GR2D-cleared background: evidence of stale fragment data.
- R16 adds FDC_CONTROL_INVALIDATE during draw-state initialization, matching
  OpenTegra. All 15 flat-color blend cases then pass. The subsequent 4x4
  texture is rejected by the existing tight-pitch guard, without submission.
- R17 uses the **identical R16 driver** and an 8x8 probe texture that satisfies
  that guard. All 30 blend cases pass in both renderers: flat MOV and TEX,
  three blend modes, five alphas, two overlapping layers, **11,760 RGBA
  component comparisons**, tolerance 4/255 and an 8-bit alpha visual. The
  existing combined texture/text/depth/mipmap regression also passes.
  Native Molecule then stops at the restricted mip-sampler assertion.
  Narrow level-zero textures such as 4x4 remain unsupported; changing the
  probe size is not a driver fix for that limitation.

`results.json` records the successful and failed steps, including skipped
later tests. Every GPU series stops on its first assertion, wrong sample or
new kernel fault. Independent kernel watches, process deadlines, own-process
cleanup and pre/post protected-file hashes remained clean. These are short
runs, not long-term stability or conformance results.

- R18 adds restricted trilinear filtering using LINEAR_BETWEEN and logs the
  actual sampler state. CPU tests pass all eight within/between/magnification
  combinations. Software and Tegra each pass all 50 rectangular 32x8, six-level
  mip cases: quarter-LOD steps from 0 through 5, both within-level filters,
  **9,800 RGBA component comparisons**, tolerance 8/255 for LOD quantization.
  Combined regression and all 30 blend cases pass again. Molecule's actual
  remaining blocker is now precise: `lod_bias=+0.25` (linear within/between,
  minLOD 0, maxLOD 1000, a 512x32 texture with levels 0..9). Bias remains
  rejected before submission; the native application still has no first frame.

## Tests included

`compat-regression.c` includes the actual compiler/emitter and exercises
2,560 finite TXP coordinate cases, native Molecule shader schedules, poisoned
metadata, exact fragment uniform upload, and 12,312 source-over component
comparisons across both source-register banks and all three blend modes.
It also checks encoded D-enable and final DW/PSEQ command ordering.
The known-unverified uniform TXP draw guard stays enabled.

Use `check-cpu.py --help` with an existing Mesa cross-build, the matching
R16 or R18 driver source/archive, ARM musl sysroot and QEMU. It opens no GPU device.
`cpu-results.json` and `sampler-cpu-results.json` record successful portable-harness runs.

`blend-probe.c` is the standalone RGBA8 result test used by R17. Build it via
`CC=... sh build-probe.sh <sysroot/include/link flags>`. Supply the expected
renderer as its sole argument (`Tegra` or `softpipe`) and run under an
isolated library environment and an external time limit/kernel monitor.
`mip-filter-probe.c` and `build-mip-probe.sh` provide the R18 fractional-LOD
result test; `check-cpu.py --sampler` tests the actual R18 sampler emitter.
The sources are formatted for review; formatting does not change the tests.
It does not install or select a system driver.

## Remaining work

Native Molecule still requires a sampler LOD bias. GRATE documents TXB's
bias-enable bit 12 and FP20 bias in R3 for the r0/r1/r2/r3 source layout.
A safe implementation must interpolate/project coordinates first, insert a
bias-load execution block, then issue TXB with correct ALU/DW scheduling.
A reserved uniform needs explicit user-constant collision checks; embedded
constants are another option. Keep the existing zero-bias path and verify
positive/negative fractional bias, clamps and unchanged UV pattern before
retrying the application. Do not silently drop the requested bias.

Uniform-coordinate TXP, transparent labels, arbitrary fragment arithmetic/
alpha test, all texture sizes/formats, scissor/stencil and broader API
coverage remain unfinished. The advertised max-LOD-bias capability is wider
than the implementation and also needs resolution. These short tests do not
justify a global driver switch.

References:

- [XScreenSaver](https://www.jwz.org/xscreensaver/)
- [OpenTegra framebuffer fetch, cache initialization and draw sequencing](https://github.com/grate-driver/xf86-video-opentegra/blob/master/src/gpu/gr3d.c)
- [OpenTegra post-upload PSEQ commit](https://github.com/grate-driver/xf86-video-opentegra/blob/master/src/exa/gen_shader_bin.c)
- [GRATE fragment instruction set](https://github.com/grate-driver/grate/blob/master/docs/fragment-shader-isa.md)
