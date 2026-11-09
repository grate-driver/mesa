# Experimental Tegra30 / GRATE lab (Mesa 22.2.4)

This is an experimental, isolated driver configuration, **not complete OpenGL
support**. Tested on ASUS Nexus 7 (2012, grouper E1565), ARMv7 musl,
postmarketOS 26.06, Linux 7.0.1-postmarketos-grate, Xorg/OpenTegra and Xfce.
The tested base is `4621b88ee2e0f2cdbcb76d9f14eda4922c0123f4` on `22.2.4`.
No kernel, bootloader, system Mesa or Xorg changes were required.

## What works, and what does not

The changes fix constant-buffer slot counts, vertex temporary-register limits,
GEM flag conversion, unsupported sampler-format advertisement, vertex immediate
uploads and a limited set of vertex lighting instructions. They also add range
checks before resource access, optional diagnostic logging and a smaller upload
manager with cleanup.

Validated paths include context creation/destruction, RGB upload through Mesa's
CPU format conversion, illuminated engine geometry, restricted GLSL texture
sampling, an opaque GL font atlas, depth occlusion, explicit RGBA8 mip storage
and nearest-mipmap GPU sampling across all eight levels. The current source
also includes restricted source-over blending, trilinear filtering, varying
TXP, and sampler LOD bias validated in R22. See the individual
probes and integrated showcase below for the limits of each test.

Unimplemented or unvalidated features include general sampler configurations,
transparent bitmap text, arbitrary applications, NaN/Inf semantics of arithmetic
CMP lowering, full GL conformance, long-running stability and other devices.

The engine test deliberately suppresses GLUT bitmap characters and its unused
reflection-mipmap initialization. The default LIT scene is untextured. These are
**test isolation hooks, not implementations of the missing functionality**. Do
not preload the isolation library into arbitrary applications.

## Build privately

Use an ARMv7 hardfloat musl cross toolchain and an independently populated ARMv7
sysroot matching the target. The tested toolchain was Bootlin stable 2024.05-1,
GCC 13.3.0; its archive SHA256 was
`5b34157a4c9f53064002f12f57c42836cd551c591a4be6a99b3ee201add506aa`.
The sysroot used Alpine 3.24 ARMv7 libraries, including development packages for
libdrm, X11/XCB, Xext, Xfixes, Xrandr, xshmfence, expat, zlib and libstdc++.
Host tools included Meson 1.3.2, Ninja, Python Mako and PLY. Populate this sysroot
on the host; do not upgrade or replace target packages as part of this test.

Supply a Meson cross file with the compiler's `--sysroot` argument, ARMv7 NEON
hardfloat flags, target pkg-config search paths and `[host_machine]` describing
ARM/Cortex-A9/Linux/little endian. Put the sysroot's `usr/lib` before toolchain
runtime libraries when linking; otherwise the compiler's bundled C++ runtime
may be selected instead of the target runtime.

From the Mesa checkout:

```sh
# Replace these paths with your cross file and the device user's PRIVATE prefix.
sh tools/grate-lab/build-private.sh /absolute/cross.ini /absolute/build-lab \
    /home/DEVICE_USER/.local/opt/mesa-grate-lab /absolute/stage-lab
```

The script only builds and stages on the host. It never contacts a device or
installs into system paths. Keep assertions enabled. Independently audit the
staged ELF architecture, hardfloat ABI, dependency/symbol versions and runtime
paths against the target before copying it. Do not use host `ldd` on target ELF.

Copy only the staged private prefix to the equivalent device location. Select
it per process, for example in an X session:

```sh
prefix="$HOME/.local/opt/mesa-grate-lab"
env LD_LIBRARY_PATH="$prefix/lib" LIBGL_DRIVERS_PATH="$prefix/lib/dri" \
    MESA_LOADER_DRIVER_OVERRIDE=tegra GRATE_DIAGNOSTICS=0 glxinfo -B
```

Expected renderer: `Tegra`, vendor `Grate`, accelerated `yes`. This alone does
not prove successful 3D rendering. Xorg EXA acceleration is a separate facility.
This lab uses X11 DRI2 and the GRATE kernel UAPI; it is not a port to the modern
upstream channel-map/submit UAPI.

## CPU-only shader regression

`vp-regression.c` compiles actual GRATE shader transformations and packing. It
also interprets the original TGSI independently and compares output against a
VP IR simulator for 128 finite input sets per fixture. `captured-*.tgsi` came
from the engine lighting shaders; `passthrough.tgsi` covers a simple input.

After building Mesa, use `check-vp.py` with the cross build directory, ARM
sysroot, QEMU ARM executable and staged runtime library directory. This runs
only on the host, creates no GL context and opens no DRM device. It does not
validate hardware precision, non-finite inputs or GL conformance.

## Bounded device tests and rollback

Before each new revision, verify an independent backup and a manifest of the
current graphics libraries, Xorg configuration and boot files. Have SSH access
and a recovery route independent of the experimental client. Keep all test
files under a new user directory. Do not modify system libraries, Xorg,
bootloader, kernel, partitions, login environment or autostart.

Start an independent kernel-log monitor before hardware tests. Abort the series
on any new SMMU/EMEM fault, timeout, assertion or unexpected exit; preserve logs
and diagnose before retrying. Use a fresh log directory and an external time
limit, for example `timeout -s TERM -k 2 8 ...` for an untraced client. A tracer
needs its own process-group handling; blindly wrapping strace with BusyBox
timeout can leave the traced application alive.

First test software rendering with the private library using
`LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=softpipe
MESA_LOADER_DRIVER_OVERRIDE=swrast`. Then test context lifetime and one RGB
upload on GRATE. Only proceed to geometry if these and the kernel log are clean.

To reproduce the limited engine test, compile `engine-isolation.c` as a private
ARMv7 shared library (musl, `-fPIC -shared -ldl`). Per process, set LD_PRELOAD to
that library and select the private Mesa prefix above, then launch the installed
mesa-demos 9.0.0 `engine`. Leave GRATE_CAPTURE_PATH unset for timing. Setting it
to a new private filename captures the third rendered back buffer as PPM, which
is useful for correctness checks but adds work. XGetImage of a DRI2 window can
return a stale compositor pixmap, so it is insufficient for this verification.

After the test, verify no client/tracer is left running, read the final kernel
log, check the desktop and recheck the protected-file manifest. Rollback normally
means ending the experimental process: its environment disappears and the system
stack remains the default. Do not reboot/restart Xorg or restore files
silently. Keep artifacts for diagnosis.

## Observed results (one device, short runs)

At 800x500, default illuminated engine geometry, with bitmap text and unused
reflection upload suppressed:

| Configuration | Frames / interval | FPS |
| --- | --- | --- |
| private softpipe | 15 / 5.102 s | 2.940 |
| private GRATE, 16 KiB uploader | 64 / 5.042 s | 12.693 |

Approximately 4.3x in this one comparison. No strace, detailed diagnostic output
or framebuffer capture was active during these timing intervals. Clocks and
thermal state were not controlled; these are illustrative single intervals,
not a general benchmark. Both clients returned zero after the engine's Escape
exit action. The harness had an EOF/reap classification race in one run;
actual process exit, rather than its attempted-signal label, was checked.

Earlier traced validation, with the same lighting implementation before the
uploader/logging changes, completed 3,371 GR3D class-0x60 submissions and returned
a visually verified engine framebuffer. No new SMMU/EMEM faults were observed;
149 protected system files and tracked links remained unchanged. System Mesa
remained llvmpipe by default. This does not establish long-term stability.

## Attribution and review status

Vertex immediate recording/upload was adapted from the Surface RT work shared
in https://github.com/grate-driver/mesa/issues/19 (commit `48283c9`). That work
also describes the smaller uploader. This branch is based on the older tested
22.2.4 interfaces, not an automatic application of the Mesa 26 patch.

Arithmetic CMP lowering currently targets the finite lighting subset. Review
its non-finite semantics and the abort-based diagnostic guards before treating
this as a general or merge-ready driver. The accompanying PR should remain a
draft until those limitations and upstream branch selection are resolved.

## Explicit mip storage regression

`grate_mipmap.h` computes separate offsets, pitches and sizes with overflow and
box-range checks. Uncompressed singleton 2D textures and buffers are supported.
Nonzero-level render surfaces and hardware blits remain unsupported; copies use
the Gallium CPU helper. Imported resources are level-zero only.

Run the standalone layout test on the build host:

```sh
cc -std=c99 -g -fsanitize=address,undefined \
  -Isrc/gallium/drivers/grate tools/grate-lab/mipmap-layout-test.c -o /tmp/grate-mipmap-test
/tmp/grate-mipmap-test
```

On the device, the isolated R5 probe uploaded distinct RGBA values at every
level from 128x128 to 1x1 and checked every pixel with glGetTexImage, with no
GL errors. Both softpipe and Tegra passed. No drawing, texture sampling or
glGenerateMipmap was involved. All 149 protected system-file entries stayed
unchanged and no new kernel faults were recorded.

## Restricted GLSL texture and opaque-text probes

The fragment backend and sampler-state port builds on the Surface RT patch in
[#19](https://github.com/grate-driver/mesa/pull/19). The Mesa 22.2.4 sampler-view
ownership API is handled explicitly and context cleanup releases the view.
The original probe uses a single linear four-byte-per-pixel 2D texture,
level zero, a matching natural row pitch, and sampler0. Min/mag nearest and
GLSL texture2D with input0 coordinates were tested. Broader current paths are
described in the R22 section below. This is not a general
fragment-shader compiler; other shader shapes can still abort.

`texture-probe.c` renders a four-quadrant RGBA8 image through a GLSL1.20 TEX
shader and checks12,544 interior framebuffer pixels with glReadPixels.
`text-atlas-probe.c` builds five opaque5x7 glyphs spelling GRATE in an RGBA8
atlas and checks14,400 framebuffer pixels through the same shader. Each passed
on private softpipe and Tegra; the hardware logs show a GR3D class0x60 submit
and wait success. No new SMMU/EMEM faults were recorded.

These are correctness checks, not FPS benchmarks. The text probe does not
implement glBitmap, transparent text, blending, discard or a font API.
The earlier fixed-function GL_REPLACE probe stopped on unsupported TXP
(projective texture sampling), before a texture draw. That failure remains
documented in the historical records: the original GLSL probe tests TEX
directly. Varying TXP is now implemented and separately tested with sampler
bias. Uniform-coordinate TXP remains guarded pending separate GPU validation.
Restricted nearest-mipmap sampling is tested in the showcase below;
automatic mip generation remains untested.

Compile the probes with your ARM cross compiler/sysroot, using the private
Mesa GL headers/library and target X11 library (`-lGL -lX11`). Run each with
argument `texture` and the private per-process environment above, under the
external deadline and fault monitor. They write `texture-readback.rgba` in the
working directory, so use a separate writable directory for every run.

## Fragment constant-buffer stage regression

Mesa 22.2.4 uses PIPE_SHADER_FRAGMENT=1 for Gallium callbacks, while
MESA_SHADER_FRAGMENT=4 is a different enum. The fragment uniform emitter
previously read the wrong array slot and skipped the actual upload. It now
uses PIPE_SHADER_FRAGMENT, matching set_constant_buffer.

Run `check-vp.py --uniforms` with the same build/sysroot/QEMU/runtime/archive
arguments as the vertex regression. This invokes the actual binding callback
and upload emitter with a mock stream, checks five exact command words for
(0.25, 0.75, 0, 1), and verifies that a separate vertex-buffer sentinel is ignored.
It performs no GPU calls.

The on-device `uniform-color-probe.c` passed 12,544 framebuffer pixel checks
(on softpipe and Tegra; one-byte tolerance for color conversion). It checks
a direct GLSL uniform color, not texture projection. Constant-coordinate TXP
was incorrect in the archived development revisions. The current source fixes
full FP20 destination masks and includes varying TXP; the uniform-coordinate
execution guard remains until that path has a separate result test. No full
desktop switch to these experimental GL libraries
has been made.

## Existing Xfce desktop path

[DESKTOP.md](DESKTOP.md) documents a separate XRender transparency/text test
and per-task GPU trace proving actual GR2D/GR3D submissions by the unchanged
system Xorg. This helps distinguish desktop EXA compositing from llvmpipe
reported for a default OpenGL context. No experimental Mesa desktop switch
was needed for that result.

## Integrated OpenGL showcase and GPU mip sampling

[showcase/README.md](showcase/README.md) contains a standalone textured-gear
application with an opaque OpenGL text atlas, Z-buffer tests and real GPU mip
selection. It validates its framebuffer before showing PASS. Both private
softpipe and Tegra passed all individual checks, the combined test and the
animated scene. The application and all libraries remain in a private prefix.

The mip sampler uses the 16-byte linear row alignment used by libgrate's mip
allocator, with checked offsets and total BO size. Non-mip resources retain
the previous layout. GPU descriptors enable mipmaps and set the actual last
level. Supported configurations remain complete level-zero POT RGBA8 views,
minimum LOD zero, and max LOD at least the resource last level. Nearest and
linear mip selection are tested. The current driver supports finite sampler0
LOD bias from -16 through +16, with nonzero-bias execution rejected when the
shader reads CONST[7] or indirectly addressed uniforms. This conservatively
protects the borrowed uniform31 slot. Zero-bias shader/uniform behavior stays
unchanged. Partial mip views and narrow, padded level-zero textures such as
4x4 are still rejected. Other configurations stop explicitly.

CPU resource and sampler command regressions can be run through
`check-vp.py --resources` and `check-vp.py --sampler` with the same build,
sysroot, QEMU, runtime and archive arguments as the vertex test. The resource
test exercises actual create/map/copy code with guarded mock BOs; the sampler
test checks actual emitted descriptors and the unchanged level-zero path.
`captured-demo.tgsi` adds the actual MVP vertex program to the vertex fixtures.

## Complete source and validation index

[INDEX.md](INDEX.md) links the driver lab, showcase, machine-readable device
results, the later 20-second display run and historical TXP/compatibility
snapshots. The current driver includes the R25 implementation below;
historical cumulative patches must only be applied to their documented base.


## Sampler LOD bias and source-over (R22)

The driver now selects a separate TXB variant for nonzero sampler bias, after
coordinate interpolation and projective division. It loads bias into FP20
R3 and enables TEX bias bit12. Both ALU destination write bits must be set:
mask00 discards the result. This also corrects temporary-coordinate MOV masks.
The implementation keeps the zero-bias variant and checks uniform-slot
collisions before submission. No requested bias is silently dropped.

The [Molecule investigation](research/molecule/README.md) includes portable
`check-cpu.py --bias`, a standalone `lod-bias-probe.c`, and pass/fail records.
The CPU test checks exact packed masks, signed upload, stage scheduling,
restoration and collision/nonfinite/range guards. In both private softpipe
and Tegra, **63,936 RGBA component comparisons pass**: nine biases including
-16,-2,-1,-0.25,0,+0.25,+1,+2,+16; six base LODs; nearest/linear within-level
filters; TEX and projective TXP; and TXP multiplied by a uniform tint with
two source-over layers. A complete four-level quadrant texture separately
checks that magnifying biases preserve UV. Tolerances are 8/255 for LOD
quantization and 4/255 for spatial patterns, with an 8-bit alpha visual.

Existing combined texture/text/depth/mipmap, 11,760-component blend and
9,800-component trilinear regressions pass again. Source-over supports only
additive premultiplied, straight, and straight-RGB/separate-ONE-alpha modes,
full RGBA mask and one render target; unsupported combinations are rejected.
Fragment cache invalidation prevents stale destinations across GR2D clears.

R25 now renders complete native Molecule frames, with lit red oxygen and two
white hydrogens confirmed by readback. A bounded indexed-draw implementation
handles the previous 7050-index blocker. The independent 49-case draw probe
passes 2,248,724 RGBA components on both Tegra and softpipe. Native labels and
titles also render correctly in R26. R27 shells render incorrectly as opaque;
general shader coverage and
long-running reliability remain unvalidated. All tests remain isolated per
process; no system Mesa switch. See the [native results and limitations](research/molecule/README.md).
