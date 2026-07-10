flacsources = flag.c

if HAVE_FLAC

flacltlibs = libinput_flac.la
flacbuildsources = $(flacsources)

else

flacltlibs =
flacbuildsources =

endif

EXTRA_DIST = $(flacsources)

lib_LTLIBRARIES = $(flacltlibs)

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

AM_CPPFLAGS = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libinput_flac_la_LDFLAGS = @PLUGIN_LDFLAGS@
libinput_flac_la_LIBADD = @GTK_LIBS@
libinput_flac_la_SOURCES = $(flacbuildsources)

