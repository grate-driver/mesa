#include <stdio.h>

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "util/u_transfer.h"
#include "util/u_inlines.h"

#include "grate_common.h"
#include "grate_context.h"
#include "grate_resource.h"
#include "grate_screen.h"

#include "host1x01_hardware.h"
#include "tgr_3d.xml.h"

#include "opentegra_drm.h"
#include "opentegra_lib.h"

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
   struct grate_context *context = grate_context(pcontext);
   struct grate_resource *resource = grate_resource(presource);
   void *ret = NULL;
   struct pipe_transfer *ptrans;

   if (usage & PIPE_TRANSFER_MAP_DIRECTLY)
      return NULL;

   ptrans = slab_alloc(&context->transfer_pool);
   if (!ptrans)
      return NULL;

   if (drm_tegra_bo_map(resource->bo, &ret))
      return NULL;

   memset(ptrans, 0, sizeof(*ptrans));

   pipe_resource_reference(&ptrans->resource, presource);
   ptrans->resource = presource;
   ptrans->level = level;
   ptrans->usage = usage;
   ptrans->box = *box;
   ptrans->stride = resource->pitch;
   ptrans->layer_stride = ptrans->stride;
   *transfer = ptrans;

   return (uint8_t *)ret +
          box->y * resource->pitch +
          box->x * util_format_get_blocksize(presource->format);
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
   struct grate_context *context = grate_context(pcontext);

   drm_tegra_bo_unmap(grate_resource(transfer->resource)->bo);

   pipe_resource_reference(&transfer->resource, NULL);
   slab_free(&context->transfer_pool, transfer);
}

static const struct u_resource_vtbl grate_resource_vtbl = {
   .resource_get_handle = grate_resource_get_handle,
   .resource_destroy = grate_resource_destroy,
   .transfer_map = grate_resource_transfer_map,
   .transfer_flush_region = grate_resource_transfer_flush_region,
   .transfer_unmap = grate_resource_transfer_unmap,
};

int
grate_pixel_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_A8_UNORM:
      return TGR3D_PIXEL_FORMAT_A8;
   case PIPE_FORMAT_L8_UNORM:
      return TGR3D_PIXEL_FORMAT_L8;
   case PIPE_FORMAT_L8A8_UNORM:
      return TGR3D_PIXEL_FORMAT_LA88;
   case PIPE_FORMAT_B5G6R5_UNORM:
      return TGR3D_PIXEL_FORMAT_RGB565;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return TGR3D_PIXEL_FORMAT_RGBA5551;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return TGR3D_PIXEL_FORMAT_RGBA4444;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return TGR3D_PIXEL_FORMAT_RGBA8888;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return TGR3D_PIXEL_FORMAT_RGBA_FP32;
   case PIPE_FORMAT_S8_UINT:
      return TGR3D_PIXEL_FORMAT_S8;
   case PIPE_FORMAT_Z16_UNORM:
      return TGR3D_PIXEL_FORMAT_D16_LINEAR;
   default:
      return -1;
   }
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
   if (template->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
                         PIPE_BIND_SCANOUT | PIPE_BIND_DEPTH_STENCIL)) {
      if (template->bind & PIPE_BIND_DEPTH_STENCIL)
         resource->pitch = align(resource->pitch, 256);
      else
         resource->pitch = align(resource->pitch, 32);

      flags = DRM_TEGRA_GEM_CREATE_BOTTOM_UP;
   }

   if (template->target != PIPE_BUFFER) {
      /* pick pixel-format */
      int format = grate_pixel_format(template->format);
      assert(format >= 0);
      resource->format = format;
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
   int err, format;

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

   format = grate_pixel_format(template->format);
   assert(format >= 0);
   resource->format = format;

   return &resource->base.b;
}

void
grate_screen_resource_init(struct pipe_screen *pscreen)
{
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
   int err, value;
   struct grate_context *context = grate_context(pcontext);
   struct grate_channel *gr2d = context->gr2d;
   struct grate_resource *dst, *src;

   dst = grate_resource(info->dst.resource);
   src = grate_resource(info->src.resource);

   err = grate_stream_begin(&gr2d->stream);
   if (err < 0) {
      fprintf(stderr, "grate_stream_begin() failed: %d\n", err);
      return;
   }

   grate_stream_push_setclass(&gr2d->stream, HOST1X_CLASS_GR2D);

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x009, 0x9));
   grate_stream_push(&gr2d->stream, 0x0000003a);            /* 0x009 - trigger */
   grate_stream_push(&gr2d->stream, 0x00000000);            /* 0x00c - cmdsel */

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x01e, 0x7));
   grate_stream_push(&gr2d->stream, 0x00000000);            /* 0x01e - controlsecond */
   /*
    * [20:20] source color depth (0: mono, 1: same)
    * [17:16] destination color depth (0: 8 bpp, 1: 16 bpp, 2: 32 bpp)
    */

   value = 1 << 20;
   switch (util_format_get_blocksize(dst->base.b.format)) {
   case 1:
      value |= 0 << 16;
      break;
   case 2:
      value |= 1 << 16;
      break;
   case 4:
      value |= 2 << 16;
      break;
   default:
      assert(0);
   }

   grate_stream_push(&gr2d->stream, value);                 /* 0x01f - controlmain */
   grate_stream_push(&gr2d->stream, 0x000000cc);            /* 0x020 - ropfade */

   grate_stream_push(&gr2d->stream, host1x_opcode_nonincr(0x046, 1));

   /*
    * [20:20] destination write tile mode (0: linear, 1: tiled)
    * [ 0: 0] tile mode Y/RGB (0: linear, 1: tiled)
    */
   value = (dst->tiled << 20) | src->tiled;
   grate_stream_push(&gr2d->stream, value);                 /* 0x046 - tilemode */

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x02b, 0xe149));
   grate_stream_push_reloc(&gr2d->stream, dst->bo, 0);      /* 0x02b - dstba */

   grate_stream_push(&gr2d->stream, dst->pitch);            /* 0x02e - dstst */

   grate_stream_push_reloc(&gr2d->stream, src->bo, 0);      /* 0x031 - srcba */

   grate_stream_push(&gr2d->stream, src->pitch);            /* 0x033 - srcst */

   value = info->dst.box.height << 16 | info->dst.box.width;
   grate_stream_push(&gr2d->stream, value);                 /* 0x038 - dstsize */

   value = info->src.box.y << 16 | info->src.box.x;
   grate_stream_push(&gr2d->stream, value);                 /* 0x039 - srcps */

   value = info->dst.box.y << 16 | info->dst.box.x;
   grate_stream_push(&gr2d->stream, value);                 /* 0x03a - dstps */

   grate_stream_end(&gr2d->stream);

   grate_stream_flush(&gr2d->stream);
}

static uint32_t
pack_color(enum pipe_format format, const float *rgba)
{
   union util_color uc;
   util_pack_color(rgba, format, &uc);
   return uc.ui[0];
}

static int
fill(struct grate_channel *gr2d,
           struct grate_resource *dst,
           uint32_t fill_value, int blocksize,
           unsigned dstx, unsigned dsty,
           unsigned width, unsigned height)
{
   uint32_t value;
   int err;

   err = grate_stream_begin(&gr2d->stream);
   if (err < 0) {
      fprintf(stderr, "grate_stream_begin() failed: %d\n", err);
      return -1;
   }

   grate_stream_push_setclass(&gr2d->stream, HOST1X_CLASS_GR2D);

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x09, 0x09));
   grate_stream_push(&gr2d->stream, 0x0000003a);           /* 0x009 - trigger */
   grate_stream_push(&gr2d->stream, 0x00000000);           /* 0x00C - cmdsel */

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x1e, 0x07));
   grate_stream_push(&gr2d->stream, 0x00000000);           /* 0x01e - controlsecond */

   value  = 1 << 6; /* fill mode */
   value |= 1 << 2; /* turbofill */
   switch (blocksize) {
   case 1:
      value |= 0 << 16;
      break;
   case 2:
      value |= 1 << 16;
      break;
   case 4:
      value |= 2 << 16;
      break;
   default:
      unreachable("invalid blocksize");
   }
   grate_stream_push(&gr2d->stream, value);           /* 0x01f - controlmain */

   grate_stream_push(&gr2d->stream, 0x000000cc);      /* 0x020 - ropfade */

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x2b, 0x09));
   grate_stream_push_reloc(&gr2d->stream, dst->bo, 0);/* 0x02b - dstba */
   grate_stream_push(&gr2d->stream, dst->pitch);      /* 0x02e - dstst */

   grate_stream_push(&gr2d->stream, host1x_opcode_nonincr(0x35, 1));

   grate_stream_push(&gr2d->stream, fill_value);           /* 0x035 - srcfgc */

   grate_stream_push(&gr2d->stream, host1x_opcode_nonincr(0x46, 1));
   grate_stream_push(&gr2d->stream, dst->tiled << 20);     /* 0x046 - tilemode */

   grate_stream_push(&gr2d->stream, host1x_opcode_mask(0x38, 0x05));
   grate_stream_push(&gr2d->stream, height << 16 | width); /* 0x038 - dstsize */
   grate_stream_push(&gr2d->stream, dsty << 16 | dstx);    /* 0x03a - dstps */
   grate_stream_end(&gr2d->stream);

   grate_stream_flush(&gr2d->stream);

   return 0;
}

static void
grate_clear(struct pipe_context *pcontext, unsigned int buffers,
            const union pipe_color_union *color, double depth,
            unsigned int stencil)
{
   struct grate_context *context = grate_context(pcontext);
   struct pipe_framebuffer_state *fb;

   fb = &context->framebuffer.base;

   if (buffers & PIPE_CLEAR_COLOR) {
      int i;
      for (i = 0; i < fb->nr_cbufs; ++i) {
         struct pipe_surface *dst = fb->cbufs[i];
         if (fill(context->gr2d, grate_resource(dst->texture),
                  pack_color(dst->format, color->f),
                  util_format_get_blocksize(dst->format),
                  0, 0, dst->width, dst->height) < 0)
            return;
      }
   }

   if (buffers & PIPE_CLEAR_DEPTH || buffers & PIPE_CLEAR_STENCIL) {
      /* TODO: handle the case where both are not set! */
      if (fill(context->gr2d, grate_resource(fb->zsbuf->texture),
               util_pack_z_stencil(fb->zsbuf->format, depth, stencil),
               util_format_get_blocksize(fb->zsbuf->format),
               0, 0, fb->zsbuf->width, fb->zsbuf->height) < 0)
         return;
   }
}

static void
grate_clear_render_target(struct pipe_context *pipe,
                          struct pipe_surface *dst,
                          const union pipe_color_union *color,
                          unsigned dstx, unsigned dsty,
                          unsigned width, unsigned height,
                          bool render_condition_enabled)
{
   assert(!render_condition_enabled);
   fill(grate_context(pipe)->gr2d, grate_resource(dst->texture),
        pack_color(dst->format, color->f), util_format_get_blocksize(dst->format),
        dstx, dsty, width, height);
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
   assert(!render_condition_enabled);
   fill(grate_context(pipe)->gr2d, grate_resource(dst->texture),
        util_pack_z_stencil(dst->format, depth, stencil),
        util_format_get_blocksize(dst->format),
        dstx, dsty, width, height);
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
