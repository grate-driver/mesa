#include <stdio.h>

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"
#include "util/u_inlines.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_resource.h"
#include "tegra_screen.h"

#include <libdrm/tegra_drm.h>
#include <libdrm/tegra.h>

/*
 * XXX Required to access winsys_handle internals. Should go away in favour
 * of some abstraction to handle handles in a Tegra-specific winsys
 * implementation.
 */
#include "state_tracker/drm_driver.h"


static boolean
tegra_resource_get_handle(struct pipe_screen *pscreen,
                          struct pipe_resource *presource,
                          struct winsys_handle *handle)
{
   struct tegra_resource *resource = tegra_resource(presource);
   int err;

   if (handle->type == DRM_API_HANDLE_TYPE_SHARED) {
      err = drm_tegra_bo_get_name(resource->bo, &handle->handle);
      if (err < 0) {
         fprintf(stderr, "drm_tegra_bo_get_name() failed: %d\n", err);
         return FALSE;
      }
   } else if (handle->type == DRM_API_HANDLE_TYPE_KMS) {
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
tegra_resource_destroy(struct pipe_screen *pscreen,
                       struct pipe_resource *presource)
{
   struct tegra_resource *resource = tegra_resource(presource);

   drm_tegra_bo_unref(resource->bo);
   FREE(resource);
}

static void *
tegra_resource_transfer_map(struct pipe_context *pcontext,
                            struct pipe_resource *presource,
                            unsigned level, unsigned usage,
                            const struct pipe_box *box,
                            struct pipe_transfer **transfer)
{
   unimplemented();

   return NULL;
}

static void
tegra_resource_transfer_flush_region(struct pipe_context *pcontext,
                                     struct pipe_transfer *transfer,
                                     const struct pipe_box *box)
{
   unimplemented();
}

static void
tegra_resource_transfer_unmap(struct pipe_context *pcontext,
                              struct pipe_transfer *transfer)
{
   unimplemented();
}

static const struct u_resource_vtbl tegra_resource_vtbl = {
   .resource_get_handle = tegra_resource_get_handle,
   .resource_destroy = tegra_resource_destroy,
   .transfer_map = tegra_resource_transfer_map,
   .transfer_flush_region = tegra_resource_transfer_flush_region,
   .transfer_unmap = tegra_resource_transfer_unmap,
};

static boolean
tegra_screen_can_create_resource(struct pipe_screen *pscreen,
                                 const struct pipe_resource *template)
{
   return TRUE;
}

static struct pipe_resource *
tegra_screen_resource_create(struct pipe_screen *pscreen,
                             const struct pipe_resource *template)
{
   struct tegra_screen *screen = tegra_screen(pscreen);
   struct tegra_resource *resource;
   uint32_t flags = 0, height, size;
   int err;

   resource = CALLOC_STRUCT(tegra_resource);
   if (!resource)
      return NULL;

   resource->base.b = *template;

   pipe_reference_init(&resource->base.b.reference, 1);
   resource->base.vtbl = &tegra_resource_vtbl;
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
tegra_screen_resource_from_handle(struct pipe_screen *pscreen,
                                  const struct pipe_resource *template,
                                  struct winsys_handle *handle,
                                  unsigned usage)
{
   struct tegra_screen *screen = tegra_screen(pscreen);
   struct tegra_resource *resource;
   int err;

   resource = CALLOC_STRUCT(tegra_resource);
   if (!resource)
      return NULL;

   resource->base.b = *template;

   pipe_reference_init(&resource->base.b.reference, 1);
   resource->base.vtbl = &tegra_resource_vtbl;
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
tegra_screen_resource_init(struct pipe_screen *pscreen)
{
   pscreen->can_create_resource = tegra_screen_can_create_resource;
   pscreen->resource_create = tegra_screen_resource_create;
   pscreen->resource_from_handle = tegra_screen_resource_from_handle;
   pscreen->resource_get_handle = u_resource_get_handle_vtbl;
   pscreen->resource_destroy = u_resource_destroy_vtbl;
}

static void
tegra_resource_copy_region(struct pipe_context *pcontext,
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
tegra_blit(struct pipe_context *pcontext, const struct pipe_blit_info *info)
{
   unimplemented();
}

static void
tegra_clear(struct pipe_context *pcontext, unsigned int buffers,
            const union pipe_color_union *color, double depth,
            unsigned int stencil)
{
   unimplemented();
}

static void
tegra_clear_render_target(struct pipe_context *pipe,
                          struct pipe_surface *dst,
                          const union pipe_color_union *color,
                          unsigned dstx, unsigned dsty,
                          unsigned width, unsigned height,
                          bool render_condition_enabled)
{
   unimplemented();
}

static void
tegra_clear_depth_stencil(struct pipe_context *pipe,
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
tegra_flush_resource(struct pipe_context *ctx, struct pipe_resource *resource)
{
   unimplemented();
}

void
tegra_context_resource_init(struct pipe_context *pcontext)
{
   pcontext->transfer_map = u_transfer_map_vtbl;
   pcontext->transfer_flush_region = u_transfer_flush_region_vtbl;
   pcontext->transfer_unmap = u_transfer_unmap_vtbl;
   pcontext->buffer_subdata = u_default_buffer_subdata;
   pcontext->texture_subdata = u_default_texture_subdata;

   pcontext->resource_copy_region = tegra_resource_copy_region;
   pcontext->blit = tegra_blit;
   pcontext->clear = tegra_clear;
   pcontext->flush_resource = tegra_flush_resource;
   pcontext->clear_render_target = tegra_clear_render_target;
   pcontext->clear_depth_stencil = tegra_clear_depth_stencil;
}
