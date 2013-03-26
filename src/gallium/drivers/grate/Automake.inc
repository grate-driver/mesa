if HAVE_GALLIUM_GRATE

TARGET_DRIVERS += tegra
TARGET_CPPFLAGS += -DGALLIUM_GRATE
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/tegra/drm/libtegradrm.la \
	$(top_builddir)/src/gallium/drivers/grate/libgrate.la \
	$(GRATE_LIBS) \
	$(LIBDRM_LIBS)

endif
