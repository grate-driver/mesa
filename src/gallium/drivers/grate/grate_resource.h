#ifndef GRATE_RESOURCE_H
#define GRATE_RESOURCE_H

#include "pipe/p_screen.h"
#include "grate_mipmap.h"
#include "util/u_transfer.h"

struct grate_resource {
   struct pipe_resource b;
   struct drm_tegra_bo *bo;
   unsigned int pitch;
   struct grate_mip_layout mip;
   unsigned int tiled : 1;
   unsigned int format : 5;
};

static inline struct grate_resource *
grate_resource(struct pipe_resource *resource)
{
   return (struct grate_resource *)resource;
}

int
grate_pixel_format(enum pipe_format format);

void
grate_context_resource_init(struct pipe_context *pcontext);

void
grate_screen_resource_init(struct pipe_screen *pscreen);

#endif
