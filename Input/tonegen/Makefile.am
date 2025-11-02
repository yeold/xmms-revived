lib_LTLIBRARIES = libtonegen.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

libtonegen_la_LDFLAGS = @PLUGIN_LDFLAGS@

libtonegen_la_SOURCES = tonegen.c
AM_CPPFLAGS = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)

