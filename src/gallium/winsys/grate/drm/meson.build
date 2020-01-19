# Copyright © 2018 NVIDIA CORPORATION
# SPDX-License-Identifier: MIT

libgratedrm = static_library(
  'gratedrm',
  'grate_drm_winsys.c',
  include_directories : [
    inc_include, inc_src, inc_gallium, inc_gallium_aux, inc_gallium_drivers,
    inc_gallium_winsys
  ],
  dependencies : [dep_libdrm, dep_libdrm_tegra],
)

driver_grate = declare_dependency(
  compile_args : '-DGALLIUM_GRATE',
  link_with : libgratedrm,
)
