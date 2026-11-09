# Tegra30 GRATE lab contents

- [Build, isolated staging and driver limitations](README.md)
- [OpenGL showcase: textured gears, text, depth and mipmaps](showcase/README.md)
- [Recorded validation and the latest 20-second run](validation/2026-11-09/README.md)
- [Machine-readable results](validation/2026-11-09/results.json)
- [Existing Xfce desktop XRender/EXA hardware proof](DESKTOP.md)
- [Native Molecule, source-over blending and cache investigation](research/molecule/README.md)
- [Archived, unfinished TXP investigation](research/txp/README.md)

The current experimental driver includes the R21/R22 source-over, trilinear
sampler and LOD-bias implementation, with explicit guards and recorded result
tests. Historical cumulative patches are documentation artifacts against their
stated older base; do not apply them over the current implementation.
