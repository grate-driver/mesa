# Copyright © 2017 Erik Faye-Lund
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

files_libtegra = files(
  'class_ids.h',
  'host1x01_hardware.h',
  'hw_host1x01_uclass.h',
  'tegra_common.h',
  'tegra_compiler_vpe.c',
  'tegra_compiler.h',
  'tegra_context.c',
  'tegra_context.h',
  'tegra_draw.c',
  'tegra_draw.h',
  'tegra_program.c',
  'tegra_program.h',
  'tegra_resource.c',
  'tegra_resource.h',
  'tegra_screen.c',
  'tegra_screen.h',
  'tegra_state.c',
  'tegra_state.h',
  'tegra_stream.c',
  'tegra_stream.h',
  'tegra_surface.c',
  'tegra_surface.h',
  'vpe_ir.c',
  'vpe_ir.h'
)

libtegra = static_library(
  'tegra',
  [files_libtegra],
  include_directories : [
    inc_src, inc_include, inc_gallium, inc_gallium_aux,
    inc_gallium_drivers, inc_drm_uapi,
  ],
  c_args : [c_vis_args],
  cpp_args : [cpp_vis_args],
  dependencies : [dep_libdrm, dep_libdrm_tegra],
  build_by_default : false,
)

driver_tegra = declare_dependency(
  compile_args : '-DGALLIUM_TEGRA',
  link_with : [libtegra, libtegrawinsys],
)
