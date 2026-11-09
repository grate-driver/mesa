# Device validation, 9 November 2026

`results.json` records the successful R11 suite, launcher checks and a later
20-second display run. The text files are exact application-output lines
selected from the retained local logs. They exclude host-specific paths,
credentials, full kernel dumps and personal backup data.

The source of the tested application is in `../../showcase/`. The default
driver and showcase do not apply the archived TXP experiments.

The suite contains 12 runs: texture, opaque text, depth, mipmap, combined
self-test and animation, each on private softpipe and Tegra. All passed.
A separate launcher check and a later 20-second hardware display run also
passed. In that later run, the panel was woken and the launcher verified Tegra;
the application produced 580 frames in 20.022 seconds, averaging 28.968 FPS.

The short paired intervals measured 24.730 FPS on Tegra and 0.788 FPS on
private softpipe. The later hardware-only measurement is a separate run.
These results do not benchmark the default system llvmpipe, control clock
rates or temperature, or prove full OpenGL conformance.

All final system manifests matched the verified backup: 149 protected files
and the complete recorded manifest were unchanged. No new kernel faults or
leftover test processes were found. System graphics and boot settings were
not changed. A system reboot was not performed.
