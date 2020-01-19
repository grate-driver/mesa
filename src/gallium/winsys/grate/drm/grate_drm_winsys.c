/*
 * Copyright © 2014-2018 NVIDIA Corporation
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>

#include "util/u_debug.h"

#include "grate/grate_screen.h"

#include <libdrm/tegra.h>

struct pipe_screen *tegra_drm_screen_create(int fd);

struct pipe_screen *tegra_drm_screen_create(int fd)
{
  struct drm_tegra *drm;
  int err = drm_tegra_new(&drm, fd);
  if (err < 0)
          return NULL;

  return grate_screen_create(drm);
}
