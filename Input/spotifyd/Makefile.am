spotifydsources = spotifyd.c

if HAVE_SPOTIFYD
spotifydltlibs = libinput_spotifyd.la
spotifydbuildsources = $(spotifydsources)
endif

EXTRA_DIST = $(spotifydsources)

lib_LTLIBRARIES = $(spotifydltlibs)

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

AM_CPPFLAGS = @GTK_CFLAGS@ @GIO_FLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libinput_spotifyd_la_LDFLAGS = @PLUGIN_LDFLAGS@
libinput_spotifyd_la_LIBADD = @GTK_LIBS@ @GIO_LIBS@
libinput_spotifyd_la_SOURCES = $(spotifydbuildsources)