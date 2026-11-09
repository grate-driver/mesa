# Distinguish Xfce compositing from Gallium OpenGL

The Nexus7 test system already has an accelerated desktop path through
Xorg/OpenTegra EXA. Its xfwm4 4.20 compositor is enabled, using XRender with
XPresent synchronization. This path does not require selecting experimental
Gallium GRATE system-wide. `glxinfo` reporting llvmpipe describes the tested
OpenGL context, not whether XRender uses the GPU.

A separate test used the **unchanged system** Xorg/OpenTegra and Xfce:

- `desktop-xrender-probe.c` composites a half-alpha512x512ARGB32 image over
  blue, adds five A8 glyphs, checks all262,144pixels with one-byte rounding
  tolerance using XGetImage on its privatepixmap, and performs12updates/moves
  of its ownwindow. Zero mismatches,82foregroundglyphpixels, exit0. The X11
  compositor selection had a nonzeroowner matching the windowmanager.
- `desktop-gpu-meter.c` samples the kernel `drm_sched_job_queue` tracepoint
  for the Xorg mainthread through a privateperfFD. Sixseconds recorded895jobs
  on`54180000.gr3d` and1435on`54140000.gr2d`, with0lostevents. These counts
  include both test and ordinaryXorgactivity, not per-frame or per-glyph
  attribution. The first500records are printed, allrecords are counted.
- No new kernel faults, unchanged149protected systemfile entries, unchanged
  complete backupmanifest, same Xorg/WM/session processes and no testprocesses
  left. No kernel/Xorg/WM/Mesa/autostart settings were changed.

This verifies actualGPU submissions from the existing X11 desktopserver,
in addition to correct XRendertransparency and text output. It is **not** a
claim that arbitrary OpenGL applications are accelerated or that GRATE's
transparent bitmap text, blending or generalshader support is complete.
Visible desktopFPS and long-termstability were not measured.

## Reproduce in an isolated test

Compile for the target with its X11/Xrenderheaders and libraries:

```sh
$CC desktop-xrender-probe.c -o desktop-xrender-probe -lXrender -lX11
$CC desktop-gpu-meter.c -o desktop-gpu-meter
```

Use the boundedprocess/kernel-fault monitoring and verifiedbackup workflow
from README.md. Run the XRenderprobe as the desktopuser with the existing
DISPLAY/XAUTHORITY, in a fresh writable directory. It writes its own
`xrender-readback.ppm`; no GLlibrary override is needed. Its only window is
closed at exit and its pixmaps are freed.

For the meter, read the tracepoint id and format at
`/sys/kernel/tracing/events/gpu_scheduler/drm_sched_job_queue/`. Supply
`EVENT_ID XORG_MAIN_PID 6` as arguments with sufficient perfpermissions.
Wait for `TRACE READY` before starting the probe. This helper expects the
field layout recorded on Linux7.0.1-postmarketos-grate: variable name at raw
offset8, variabledev at20, fencecontext/seqno at24/32 andclientid at40.
**Check the format before using it on another kernel.** It does not write
globaltracefsconfiguration or stop/ptrace-attach Xorg. Its per-taskevent has
inheritance disabled; a first developmentattempt with inheritance was
rejected by perf mmap and did not run the renderingprobe. The perfFD closes
after the bounded interval.
