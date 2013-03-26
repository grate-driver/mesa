#include <stdio.h>

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"
#include "util/u_inlines.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_resource.h"
#include "grate_screen.h"

#include <tegra_drm.h>
#include <libdrm/tegra.h>

/*
 * XXX Required to access winsys_handle internals. Should go away in favour
 * of some abstraction to handle handles in a Tegra-specific winsys
 * implementation.
 */
#include "state_tracker/drm_driver.h"


static bool
grate_resource_get_handle(struct pipe_screen *pscreen,
                          struct pipe_resource *presource,
                          struct winsys_handle *handle)
{
   struct grate_resource *resource = grate_resource(presource);
   int err;

   if (handle->type == WINSYS_HANDLE_TYPE_SHARED) {
      err = drm_tegra_bo_get_name(resource->bo, &handle->handle);
      if (err < 0) {
         fprintf(stderr, "drm_tegra_bo_get_name() failed: %d\n", err);
         return FALSE;
      }
   } else if (handle->type == WINSYS_HANDLE_TYPE_KMS) {
      err = drm_tegra_bo_get_handle(resource->bo, &handle->handle);
      if (err < 0) {
         fprintf(stderr, "drm_tegra_bo_get_handle() failed: %d\n", err);
         return FALSE;
      }
   } else {
      fprintf(stdout, "unsupported handle type: %d\n", handle->type);
      return FALSE;
   }

   handle->stride = resource->pitch;
   return TRUE;
}

static void
grate_resource_destroy(struct pipe_screen *pscreen,
                       struct pipe_resource *presource)
{
   struct grate_resource *resource = grate_resource(presource);

   drm_tegra_bo_unref(resource->bo);
   FREE(resource);
}

static void *
grate_resource_transfer_map(struct pipe_context *pcontext,
                            struct pipe_resource *presource,
                            unsigned level, unsigned usage,
                            const struct pipe_box *box,
                            struct pipe_transfer **transfer)
{
   unimplemented();

   return NULL;
}

static void
grate_resource_transfer_flush_region(struct pipe_context *pcontext,
                                     struct pipe_transfer *transfer,
                                     const struct pipe_box *box)
{
   unimplemented();
}

static void
grate_resource_transfer_unmap(struct pipe_context *pcontext,
                              struct pipe_transfer *transfer)
{
   unimplemented();
}

static const struct u_resource_vtbl grate_resource_vtbl = {
   .resource_get_handle = grate_resource_get_handle,
   .resource_destroy = grate_resource_destroy,
   .transfer_map = grate_resource_transfer_map,
   .transfer_flush_region = grate_resource_transfer_flush_region,
   .transfer_unmap = grate_resource_transfer_unmap,
};

static bool
grate_screen_can_create_resource(struct pipe_screen *pscreen,
                                 const struct pipe_resource *template)
{
   return TRUE;
}

static struct pipe_resource *
grate_screen_resource_create(struct pipe_screen *pscreen,
                             const struct pipe_resource *template)
{
   struct grate_screen *screen = grate_screen(pscreen);
   struct grate_resource *resource;
   uint32_t flags = 0, height, size;
   int err;

   resource = CALLOC_STRUCT(grate_resource);
   if (!resource)
      return NULL;

   resource->base.b = *template;

   pipe_reference_init(&resource->base.b.reference, 1);
   resource->base.vtbl = &grate_resource_vtbl;
   resource->base.b.screen = pscreen;

   resource->pitch = template->width0 * util_format_get_blocksize(template->format);
   height = template->height0;

   resource->tiled = 0;
   if (template->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_SCANOUT)) {
      resource->pitch = align(resource->pitch, 32);
      flags = DRM_TEGRA_GEM_CREATE_BOTTOM_UP;
   }

   size = resource->pitch * height;

   err = drm_tegra_bo_new(&resource->bo, screen->drm, flags, size);
   if (err < 0) {
      fprintf(stderr, "drm_tegra_bo_new() failed: %d\n", err);
      return NULL;
   }

   return &resource->base.b;
}

static struct pipe_resource *
grate_screen_resource_from_handle(struct pipe_screen *pscreen,
                                  const struct pipe_resource *template,
                                  struct winsys_handle *handle,
                                  unsigned usage)
{
   struct grate_screen *screen = grate_screen(pscreen);
   struct grate_resource *resource;
   int err;

   resource = CALLOC_STRUCT(grate_resource);
   if (!resource)
      return NULL;

   resource->base.b = *template;

   pipe_reference_init(&resource->base.b.reference, 1);
   resource->base.vtbl = &grate_resource_vtbl;
   resource->base.b.screen = pscreen;

   err = drm_tegra_bo_from_name(&resource->bo, screen->drm,
                                handle->handle, 0);
   if (err < 0) {
      fprintf(stderr, "drm_tegra_bo_from_name() failed: %d\n", err);
      FREE(resource);
      return NULL;
   }

   resource->pitch = handle->stride;

   return &resource->base.b;
}

void
grate_screen_resource_init(struct pipe_screen *pscreen)
{
   pscreen->can_create_resource = grate_screen_can_create_resource;
   pscreen->resource_create = grate_screen_resource_create;
   pscreen->resource_from_handle = grate_screen_resource_from_handle;
   pscreen->resource_get_handle = u_resource_get_handle_vtbl;
   pscreen->resource_destroy = u_resource_destroy_vtbl;
}

static void
grate_resource_copy_region(struct pipe_context *pcontext,
                           struct pipe_resource *dst,
                           unsigned int dst_level,
                           unsigned int dstx, unsigned dsty,
                           unsigned int dstz,
                           struct pipe_resource *src,
                           unsigned int src_level,
                           const struct pipe_box *box)
{
   unimplemented();
}

static void
grate_blit(struct pipe_context *pcontext, const struct pipe_blit_info *info)
{
   unimplemented();
}

static void
grate_clear(struct pipe_context *pcontext, unsigned int buffers,
            const union pipe_color_union *color, double depth,
            unsigned int stencil)
{
   unimplemented();
}

static void
grate_clear_render_target(struct pipe_context *pipe,
                          struct pipe_surface *dst,
                          const union pipe_color_union *color,
                          unsigned dstx, unsigned dsty,
                          unsigned width, unsigned height,
                          bool render_condition_enabled)
{
   unimplemented();
}

static void
grate_clear_depth_stencil(struct pipe_context *pipe,
                          struct pipe_surface *dst,
                          unsigned clear_flags,
                          double depth,
                          unsigned stencil,
                          unsigned dstx, unsigned dsty,
                          unsigned width, unsigned height,
                          bool render_condition_enabled)
{
   unimplemented();
}

static void
grate_flush_resource(struct pipe_context *ctx, struct pipe_resource *resource)
{
   unimplemented();
}

void
grate_context_resource_init(struct pipe_context *pcontext)
{
   pcontext->transfer_map = u_transfer_map_vtbl;
   pcontext->transfer_flush_region = u_transfer_flush_region_vtbl;
   pcontext->transfer_unmap = u_transfer_unmap_vtbl;
   pcontext->buffer_subdata = u_default_buffer_subdata;
   pcontext->texture_subdata = u_default_texture_subdata;

   pcontext->resource_copy_region = grate_resource_copy_region;
   pcontext->blit = grate_blit;
   pcontext->clear = grate_clear;
   pcontext->flush_resource = grate_flush_resource;
   pcontext->clear_render_target = grate_clear_render_target;
   pcontext->clear_depth_stencil = grate_clear_depth_stencil;
}
