include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libgrate.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers/grate/include \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(LIBDRM_CFLAGS)

libgrate_la_SOURCES = \
	class_ids.h \
	host1x01_hardware.h \
	hw_host1x01_uclass.h \
	grate_common.h \
	grate_context.c \
	grate_context.h \
	grate_draw.c \
	grate_draw.h \
	grate_fence.h \
	grate_resource.c \
	grate_resource.h \
	grate_screen.c \
	grate_screen.h \
	grate_state.c \
	grate_state.h \
	grate_stream.c \
	grate_stream.h \
	grate_surface.c \
	grate_surface.h
