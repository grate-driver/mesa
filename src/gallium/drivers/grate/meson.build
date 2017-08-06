# Copyright © 2017 Erik Faye-Lund
# SPDX-License-Identifier: MIT

files_libgrate = files(
  'class_ids.h',
  'host1x01_hardware.h',
  'hw_host1x01_uclass.h',
  'grate_common.h',
  'grate_context.c',
  'grate_context.h',
  'grate_draw.c',
  'grate_draw.h',
  'grate_resource.c',
  'grate_resource.h',
  'grate_screen.c',
  'grate_screen.h',
  'grate_state.c',
  'grate_state.h',
  'grate_stream.c',
  'grate_stream.h',
  'grate_surface.c',
  'grate_surface.h',
)

libgrate = static_library(
  'grate',
  [files_libgrate],
  include_directories : [
    inc_src, inc_include, inc_gallium, inc_gallium_aux,
    inc_gallium_drivers,
  ],
  c_args : [c_vis_args],
  cpp_args : [cpp_vis_args],
  dependencies : [dep_libdrm, dep_libdrm_tegra],
  build_by_default : false,
)

driver_grate = declare_dependency(
  compile_args : '-DGALLIUM_GRATE',
  link_with : [libgrate, libtegradrm],
)
