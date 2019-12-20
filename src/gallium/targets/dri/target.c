#include "target-helpers/drm_helper.h"
#include "target-helpers/sw_helper.h"

#include "dri_screen.h"

#define DEFINE_LOADER_DRM_ENTRYPOINT(drivername)                          \
const __DRIextension **__driDriverGetExtensions_##drivername(void);       \
PUBLIC const __DRIextension **__driDriverGetExtensions_##drivername(void) \
{                                                                         \
   globalDriverAPI = &galliumdrm_driver_api;                              \
   return galliumdrm_driver_extensions;                                   \
}

#if defined(GALLIUM_SOFTPIPE)

const __DRIextension **__driDriverGetExtensions_swrast(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_swrast(void)
{
   globalDriverAPI = &galliumsw_driver_api;
   return galliumsw_driver_extensions;
}

#if defined(HAVE_LIBDRM)

const __DRIextension **__driDriverGetExtensions_kms_swrast(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_kms_swrast(void)
{
   globalDriverAPI = &dri_kms_driver_api;
   return galliumdrm_driver_extensions;
}

#endif
#endif

#if defined(GALLIUM_I915)
DEFINE_LOADER_DRM_ENTRYPOINT(i915)
#endif

#if defined(GALLIUM_IRIS)
DEFINE_LOADER_DRM_ENTRYPOINT(iris)
#endif

#if defined(GALLIUM_NOUVEAU)
DEFINE_LOADER_DRM_ENTRYPOINT(nouveau)
#endif

#if defined(GALLIUM_R300)
DEFINE_LOADER_DRM_ENTRYPOINT(r300)
#endif

#if defined(GALLIUM_R600)
DEFINE_LOADER_DRM_ENTRYPOINT(r600)
#endif

#if defined(GALLIUM_RADEONSI)
DEFINE_LOADER_DRM_ENTRYPOINT(radeonsi)
#endif

#if defined(GALLIUM_VMWGFX)
DEFINE_LOADER_DRM_ENTRYPOINT(vmwgfx)
#endif

#if defined(GALLIUM_FREEDRENO)
DEFINE_LOADER_DRM_ENTRYPOINT(msm)
DEFINE_LOADER_DRM_ENTRYPOINT(kgsl)
#endif

#if defined(GALLIUM_VIRGL)
DEFINE_LOADER_DRM_ENTRYPOINT(virtio_gpu)
#endif

#if defined(GALLIUM_V3D)
DEFINE_LOADER_DRM_ENTRYPOINT(v3d)
#endif

#if defined(GALLIUM_VC4)
DEFINE_LOADER_DRM_ENTRYPOINT(vc4)
#endif

#if defined(GALLIUM_PANFROST)
DEFINE_LOADER_DRM_ENTRYPOINT(panfrost)
#endif

#if defined(GALLIUM_ETNAVIV)
DEFINE_LOADER_DRM_ENTRYPOINT(etnaviv)
#endif

#if defined(GALLIUM_TEGRA) || defined(GALLIUM_GRATE)
DEFINE_LOADER_DRM_ENTRYPOINT(tegra);
#endif

#if defined(GALLIUM_KMSRO)
DEFINE_LOADER_DRM_ENTRYPOINT(armada_drm)
DEFINE_LOADER_DRM_ENTRYPOINT(exynos)
DEFINE_LOADER_DRM_ENTRYPOINT(hx8357d)
DEFINE_LOADER_DRM_ENTRYPOINT(ili9225)
DEFINE_LOADER_DRM_ENTRYPOINT(ili9341)
DEFINE_LOADER_DRM_ENTRYPOINT(imx_drm)
DEFINE_LOADER_DRM_ENTRYPOINT(meson)
DEFINE_LOADER_DRM_ENTRYPOINT(mi0283qt)
DEFINE_LOADER_DRM_ENTRYPOINT(mxsfb_drm)
DEFINE_LOADER_DRM_ENTRYPOINT(pl111)
DEFINE_LOADER_DRM_ENTRYPOINT(repaper)
DEFINE_LOADER_DRM_ENTRYPOINT(rockchip)
DEFINE_LOADER_DRM_ENTRYPOINT(st7586)
DEFINE_LOADER_DRM_ENTRYPOINT(st7735r)
DEFINE_LOADER_DRM_ENTRYPOINT(stm)
DEFINE_LOADER_DRM_ENTRYPOINT(sun4i_drm)
#endif

#if defined(GALLIUM_LIMA)
DEFINE_LOADER_DRM_ENTRYPOINT(lima)
#endif
