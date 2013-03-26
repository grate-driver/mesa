if HAVE_GALLIUM_TEGRA

TARGET_DRIVERS += tegra
TARGET_CPPFLAGS += -DGALLIUM_TEGRA
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/tegra/drm/libtegradrm.la \
	$(top_builddir)/src/gallium/drivers/tegra/libtegra.la \
	$(TEGRA_LIBS) \
	$(LIBDRM_LIBS)

endif
