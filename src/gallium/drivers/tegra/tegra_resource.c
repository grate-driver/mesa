#include <stdio.h>

#include "util/u_inlines.h"

#include "tegra_resource.h"

static boolean
tegra_screen_can_create_resource(struct pipe_screen *pscreen,
				 const struct pipe_resource *template)
{
	bool ret = TRUE;
	fprintf(stdout, "> %s(pscreen=%p, template=%p)\n", __func__, pscreen,
		template);
	fprintf(stdout, "< %s() = %d\n", __func__, ret);
	return ret;
}

static struct pipe_resource *
tegra_screen_resource_create(struct pipe_screen *pscreen,
			     const struct pipe_resource *template)
{
	struct tegra_resource *resource;

	fprintf(stdout, "> %s(pscreen=%p, template=%p)\n", __func__, pscreen,
		template);

	resource = calloc(1, sizeof(*resource));
	if (!resource) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	resource->base = *template;

	pipe_reference_init(&resource->base.reference, 1);
	resource->base.screen = pscreen;

	fprintf(stdout, "< %s() = %p\n", __func__, &resource->base);
	return &resource->base;
}

static struct pipe_resource *
tegra_screen_resource_from_handle(struct pipe_screen *pscreen,
				  const struct pipe_resource *template,
				  struct winsys_handle *handle,
				  unsigned usage)
{
	struct tegra_resource *resource;

	fprintf(stdout, "> %s(pscreen=%p, template=%p, handle=%p, usage=%x)\n",
		__func__, pscreen, template, handle, usage);

	resource = calloc(1, sizeof(*resource));
	if (!resource) {
		fprintf(stdout, "< %s() = NULL\n", __func__);
		return NULL;
	}

	resource->base = *template;

	pipe_reference_init(&resource->base.reference, 1);
	resource->base.screen = pscreen;

	fprintf(stdout, "< %s() = %p\n", __func__, &resource->base);
	return &resource->base;
}

static boolean
tegra_screen_resource_get_handle(struct pipe_screen *pscreen,
				 struct pipe_context *pcontext,
				 struct pipe_resource *resource,
				 struct winsys_handle *handle,
				 unsigned usage)
{
	boolean ret = TRUE;
	fprintf(stdout, "> %s(pscreen=%p, pcontext=%p resource=%p, handle=%p, usage=%x)\n",
		__func__, pscreen, pcontext, resource, handle, usage);
	fprintf(stdout, "< %s() = %d\n", __func__, ret);
	return ret;
}

static void tegra_screen_resource_destroy(struct pipe_screen *pscreen,
					  struct pipe_resource *resource)
{
	fprintf(stdout, "> %s(pscreen=%p, resource=%p)\n", __func__, pscreen,
		resource);
	fprintf(stdout, "< %s()\n", __func__);
}

void tegra_screen_resource_init(struct pipe_screen *pscreen)
{
	pscreen->can_create_resource = tegra_screen_can_create_resource;
	pscreen->resource_create = tegra_screen_resource_create;
	pscreen->resource_from_handle = tegra_screen_resource_from_handle;
	pscreen->resource_get_handle = tegra_screen_resource_get_handle;
	pscreen->resource_destroy = tegra_screen_resource_destroy;
}
