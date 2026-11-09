# Native Molecule compatibility investigation

The current branch driver includes the R25 implementation and builds it
normally in the isolated lab configuration. The cumulative patches below are
**historical snapshots**, not extra patches to apply to the current source.
Full Molecule compatibility and complete OpenGL support remain unfinished.

The chosen independent application is the unmodified, signed Alpine ARMv7
XScreenSaver 6.15 `molecule` executable. The private test loads a three-atom
water PDB at 640x480 with labels, titles and electron shells disabled. A
diagnostic preload reports the real EGL renderer, counts swaps and reads
back frame 3; it does not suppress application features or GL calls.
Softpipe renders the actual red oxygen and white hydrogen atoms. Hardware now renders complete frames with the R25 draw fix. The captured third
frame shows the expected lit red oxygen and two white hydrogen atoms. The
five-second diagnostic run recorded 171 swaps; this is not a paired performance
benchmark. Labels, titles and shells were disabled in that run.
R26 then enables labels and titles: the captured image confirms H1/H2/O labels,
the yellow title and H₂O formula, in addition to the lit atoms. It recorded 104
Tegra swaps versus 12 softpipe swaps in short diagnostic runs, with different
poses and diagnostic overhead; these are not a controlled performance benchmark.
The self/blend/mip/bias regressions all pass again. See
[native-label-results.json](native-label-results.json) and the
[hardware capture](native-water-labels-grate.png) /
[software capture](native-water-labels-softpipe.png).

R27 also enables electron shells. The application exits normally and produces
77 Tegra swaps, but **visual validation fails**: the GPU shells are opaque and
occlude labels, while the software control shows transparency. This is a
remaining rendering bug; a successful process exit/readback is insufficient.
The root cause is not assigned. The next targeted tests should isolate varying
COLOR alpha, shell depth/blend state and draw ordering. See
[native-shell-results.json](native-shell-results.json),
[failed hardware image](native-water-shells-grate.png) and
[software control](native-water-shells-softpipe.png). No new kernel faults or
protected system changes occurred, and authorization/inhibitor/process cleanup
passed. Native correctness claims above apply to `--no-shells` only.

## Indexed draw batching (R23–R25)

The previous native blocker was a UINT32-indexed triangle draw with start 7806
and count 7050. R25 splits supported large primitives at an even 2046 elements,
using Mesa's topology split helper to preserve line-strip joins and triangle-strip
winding. It copies each selected index slice into bounded scratch storage before
uploading it at a 64-byte aligned pointer; indexed primitive offset and FIRST
are zero. The R24 attempt to align a resource pointer and encode the remaining
index offset produced wrong pixels and was rejected.

Verified GR3D index modes are UINT8 and UINT16. UINT32 input is converted without
loss only when each referenced index fits UINT16; larger values abort before
submission. This is restricted compatibility, not general 32-bit vertex
rebasing. Nonindexed starts use DRAW_PRIMITIVES_OFFSET. Empty draws return before
shader access, source ranges and upload ranges are checked, caller draw records
are not mutated, and the upload reference is released after synchronous submit.
Indirect, instanced, primitive-restart and nonzero base-vertex draws remain
guarded. Large fans/loops also remain guarded; there is no topology truncation.
The legacy source-buffer take-ownership contract still needs a separate review.

`draw-probe.c` independently checks 49 GL cases and **2,248,724 RGBA components**
on softpipe and Tegra: nonzero VBO/index starts, client/resource UINT8/16/32,
counts around 2047 and multiple batches through 12345, and 9000-vertex culled
triangle strips. Poison prefixes/padding expose addressing mistakes. GPU and
software pass every case, with tolerance 3/255. `draw-regression.c` includes the
actual driver emitter and compares 593,610 primitive tuples, command fields,
upload contents/references, empty draws, byte-range overflow and UINT32 narrowing
boundaries under ARM/QEMU. Select it with `check-cpu.py --draw` and the same build
arguments used for the other regression modes. `build-draw-probe.sh` uses the
same CC/sysroot conventions as the other probe scripts. See
[draw-results.json](draw-results.json) and
[draw-cpu-results.json](draw-cpu-results.json). All experiments use isolated
libraries, bounded processes, a kernel watcher, private X authorization and a
temporary sleep inhibitor; the 149-entry protected baseline remained unchanged.


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

- R19 adds TXB variants for sampler bias, a reserved-uniform collision guard,
  signed uploads and a 63,936-component probe. CPU checks passed but hardware
  ignored the requested bias. R20 tried R2 instead of R3 and failed identically.
- R21 identifies the actual defect in the upstream GRATE `texture-filter`
  shader: full FP20 ALU writes use `.lh`, both destination enables. Mask00
  discards the result. It restores bias in R3, fixes temporary-coordinate MOV
  masks and adds an exact packed-mask regression. The first 108 flat bias
  cases pass; the UV fixture then hits an incomplete-view guard.
- R22 uses the identical R21 driver and corrects the fixture to a complete
  four-level chain with magnifying biases for UV checks. All **63,936 RGBA
  bias comparisons pass on both renderers**, including projective coordinates,
  uniform color modulation and two source-over layers. Self/blend/trilinear
  regressions pass. Molecule accepts its real +0.25 sampler, submits geometry,
  then aborts at `draws->count > 0 && draws->count < 2048`, before its first
  frame. New kernel faults, protected-file changes and leftover tests: zero.

## Tests included

`compat-regression.c` includes the actual compiler/emitter and exercises
2,560 finite TXP coordinate cases, native Molecule shader schedules, poisoned
metadata, exact fragment uniform upload, and 12,312 source-over component
comparisons across both source-register banks and all three blend modes.
It also checks encoded D-enable and final DW/PSEQ command ordering.
The known-unverified uniform TXP draw guard stays enabled.

Use `check-cpu.py --help` with an existing Mesa cross-build, the matching
R21/R22 driver source/archive, ARM musl sysroot and QEMU. It opens no GPU device.
`cpu-results.json` and `sampler-cpu-results.json` record earlier harness runs.
`lod-bias-cpu-results.json` records the current regression against the actual
public driver source. `check-cpu.py --bias` targets R21/R22; its corrected
FP20-mask expectations intentionally reject the earlier broken implementation.

`blend-probe.c` is the standalone RGBA8 result test used by R17. Build it via
`CC=... sh build-probe.sh <sysroot/include/link flags>`. Supply the expected
renderer as its sole argument (`Tegra` or `softpipe`) and run under an
isolated library environment and an external time limit/kernel monitor.
`mip-filter-probe.c` and `build-mip-probe.sh` provide the R18 fractional-LOD
result test; `check-cpu.py --sampler` tests the actual R18 sampler emitter.
The sources are formatted for review; formatting does not change the tests.
It does not install or select a system driver.

## Remaining work

Sampler LOD bias is now implemented for the restricted sampler0 2D path.
Nonzero-bias variants borrow uniform31 only if the shader never reads
CONST[7] or indirect constants; otherwise execution is rejected. Zero-bias
variants keep all application uniforms. Bias loading follows projection;
full FP20 writes require both enable bits, as in the upstream filter shader.

`lod-bias-probe.c` / `build-bias-probe.sh` exercise 324 flat mip cases across
TEX, TXP and TXP*tint+two-layer source-over. That is 63,504 component samples
with 8/255 tolerance. Nine magnifying UV biases on a complete quadrant mip
chain add 432 samples with 4/255 tolerance. No UV scaling workaround is used.
Private payload debug information was stripped for transfer; all 24 allocated
ELF sections remained byte-identical (`strip-audit.json`).

Molecule still requires correct handling of empty/large draw commands. The
assertion does not identify which side of the count guard failed; that must
be diagnosed before changing the limit. Uniform-coordinate TXP remains
blocked despite the TEMP mask fix, pending its own GPU result tests.
Transparent native labels, general fragment arithmetic/alpha test, texture
formats/sizes, scissor/stencil and broader API/conformance coverage remain
unfinished. These short tests do not justify a global driver switch.

References:

- [XScreenSaver](https://www.jwz.org/xscreensaver/)
- [OpenTegra framebuffer fetch, cache initialization and draw sequencing](https://github.com/grate-driver/xf86-video-opentegra/blob/master/src/gpu/gr3d.c)
- [OpenTegra post-upload PSEQ commit](https://github.com/grate-driver/xf86-video-opentegra/blob/master/src/exa/gen_shader_bin.c)
- [GRATE fragment instruction set](https://github.com/grate-driver/grate/blob/master/docs/fragment-shader-isa.md)

- [Reference TXB shader with full FP20 destination enables](https://github.com/grate-driver/grate/blob/ce8c4d2c62880ee5ffab90fd5f4c0554d4dbcc6d/tests/grate/asm/filter_quad_fs.txt)
- [Upstream LOD-bias and mip-filter test](https://github.com/grate-driver/grate/blob/ce8c4d2c62880ee5ffab90fd5f4c0554d4dbcc6d/tests/grate/texture-filter.c)
