#include <stdio.h>
#include <stdlib.h>

#include <libdrm/tegra.h>

#include "tegra/tegra_screen.h"
#include "tegra_drm_public.h"

struct pipe_screen *tegra_drm_screen_create(int fd)
{
	struct pipe_screen *screen = NULL;
	struct drm_tegra *drm;
	int err;

	fprintf(stderr, "> %s(fd=%d)\n", __func__, fd);

	err = drm_tegra_new(&drm, fd);
	if (err < 0)
		goto out;

	screen = tegra_screen_create(drm);

out:
	fprintf(stderr, "< %s() = %p\n", __func__, screen);
	return screen;
}
