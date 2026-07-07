if HAVE_MIKMOD

lib_LTLIBRARIES = libinput_mikmod.la

endif

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

AM_CPPFLAGS = @GTK_CFLAGS@ @LIBMIKMOD_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libinput_mikmod_la_LDFLAGS = @PLUGIN_LDFLAGS@
libinput_mikmod_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ \
@LIBMIKMOD_LIBS@ @LIBMIKMOD_LDADD@ 

libinput_mikmod_la_SOURCES = drv_xmms.c mikmod-plugin.h plugin.c mikmod.xpm

