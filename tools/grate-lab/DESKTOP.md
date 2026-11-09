# Xfce compositing and Gallium OpenGL are separate paths

The Nexus 7 test system already accelerates desktop compositing through
Xorg/OpenTegra EXA. Its xfwm4 4.20 compositor is enabled and uses XRender with
XPresent synchronization. This path does not require selecting experimental
Gallium GRATE system-wide. The renderer reported by `glxinfo` describes an
OpenGL context; it does not establish whether XRender uses the GPU.

The following checks used the unchanged system Xorg/OpenTegra and Xfce:

- `desktop-xrender-probe.c` composites a half-alpha 512×512 ARGB32 image over
  blue, adds five A8 glyphs, and checks all 262,144 pixels with a one-byte
  rounding tolerance. It reads its private pixmap through XGetImage, avoiding
  stale compositor screenshots. All pixels passed, including 82 glyph pixels.
  Its own window then completed 12 updates and moves. The compositor selection
  owner matched the window manager.
- `desktop-gpu-meter.c` samples `drm_sched_job_queue` through a private perf
  event for Xorg's main thread. Six seconds recorded 895 jobs on
  `54180000.gr3d` and 1,435 on `54140000.gr2d`, with no lost events. These
  include both test and ordinary Xorg activity; they are not per-frame or
  per-glyph counts. The first 500 records are printed and all are counted.
- No new kernel faults occurred. All 149 protected system files and the complete
  recorded manifest matched the verified backup. Xorg, the window manager and
  the session remained running, and no test processes remained. No kernel,
  Xorg, window manager, Mesa or autostart settings were changed.

This verifies actual GPU submissions from the existing desktop graphics server,
in addition to correct XRender transparency and text output. It does not imply
that arbitrary OpenGL applications are accelerated or that GRATE's bitmap text,
blending and general shader support are complete. Visible desktop frame rate
and long-term stability were not measured.

## Reproduce in an isolated test

Compile for the target with its X11/XRender headers and libraries:

```sh
$CC desktop-xrender-probe.c -o desktop-xrender-probe -lXrender -lX11
$CC desktop-gpu-meter.c -o desktop-gpu-meter
```

Use the bounded process and kernel-fault monitoring workflow from README.md,
with a verified backup. Run the XRender probe as the desktop user, with the
existing DISPLAY/XAUTHORITY, in a fresh writable directory. It writes
`xrender-readback.ppm`. No GL library override is needed. Its window is closed
and its pixmaps are freed at exit.

Read the tracepoint ID and format from
`/sys/kernel/tracing/events/gpu_scheduler/drm_sched_job_queue/`. Run the meter
with arguments `EVENT_ID XORG_MAIN_PID 6` and sufficient perf permissions.
Wait for `TRACE READY` before starting the probe.

The meter expects the layout recorded on Linux 7.0.1-postmarketos-grate:
variable name at raw offset 8, variable device name at 20, fence context and
sequence number at 24 and 32, and client ID at 40. **Check the event format
before using it on another kernel.** It writes no global tracefs configuration
and does not stop or ptrace-attach Xorg. The event has inheritance disabled
and its file descriptor closes after the bounded interval. An earlier attempt
with inheritance was rejected by perf mmap before the rendering probe ran.
