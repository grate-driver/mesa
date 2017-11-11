include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libtegra.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers/tegra/include \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(LIBDRM_CFLAGS)

libtegra_la_SOURCES = \
	class_ids.h \
	host1x01_hardware.h \
	hw_host1x01_uclass.h \
	tegra_common.h \
	tegra_context.c \
	tegra_context.h \
	tegra_draw.c \
	tegra_draw.h \
	tegra_fence.h \
	tegra_program.c \
	tegra_program.h \
	tegra_resource.c \
	tegra_resource.h \
	tegra_screen.c \
	tegra_screen.h \
	tegra_state.c \
	tegra_state.h \
	tegra_stream.c \
	tegra_stream.h \
	tegra_surface.c \
	tegra_surface.h
