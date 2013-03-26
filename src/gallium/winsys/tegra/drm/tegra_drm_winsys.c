#include <stdio.h>
#include <stdlib.h>

#include <libdrm/tegra.h>

#include "grate/grate_screen.h"
#include "tegra_drm_public.h"

struct pipe_screen *tegra_drm_screen_create(int fd)
{
	struct drm_tegra *drm;
	int err = drm_tegra_new(&drm, fd);
	if (err < 0)
		return NULL;

	return grate_screen_create(drm);
}
