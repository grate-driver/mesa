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

Validated: context creation/destruction, one RGB texture upload using Mesa's CPU
format conversion, and illuminated, depth-tested engine geometry on GR3D.
Not validated/implemented: general texture sampling, mipmap storage/addressing,
bitmap text, arbitrary applications, NaN/Inf correctness of arithmetic CMP
lowering, full GL conformance, long-running stability or other devices.

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
