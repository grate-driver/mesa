#ifndef _DRM_HELPER_PUBLIC_H
#define _DRM_HELPER_PUBLIC_H

struct pipe_screen;
struct pipe_screen_config;

const char *iris_driconf_xml;
const char *radeonsi_driconf_xml;
const char *v3d_driconf_xml;
const char *virgl_driconf_xml;

struct pipe_screen *
pipe_i915_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_iris_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_nouveau_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_r300_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_r600_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_radeonsi_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_vmwgfx_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_freedreno_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_virgl_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_v3d_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_vc4_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_panfrost_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_kmsro_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_etna_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_imx_drm_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_tegra_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_grate_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_lima_create_screen(int fd, const struct pipe_screen_config *config);

struct pipe_screen *
pipe_zink_create_screen(int fd, const struct pipe_screen_config *config);


#endif /* _DRM_HELPER_PUBLIC_H */
