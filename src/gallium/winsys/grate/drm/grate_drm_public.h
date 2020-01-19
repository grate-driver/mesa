/*
 * Copyright © 2014-2018 NVIDIA Corporation
 * SPDX-License-Identifier: MIT
 */

#ifndef __TEGRA_DRM_PUBLIC_H__
#define __TEGRA_DRM_PUBLIC_H__

struct pipe_screen;

struct pipe_screen *tegra_drm_screen_create(int fd);

#endif /* __TEGRA_DRM_PUBLIC_H__ */
