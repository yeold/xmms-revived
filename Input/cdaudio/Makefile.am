cdaudiosources = cdaudio.c \
configure.c \
cdaudio.h \
cdinfo.c \
cdinfo.h \
cddb.c \
cddb.h \
http.c \
http.h

if HAVE_CDROM

cdaudioltlibs = libinput_cdaudio.la
cdaudiobuildsources = $(cdaudiosources)

else

cdaudioltlibs =
cdaudiobuildsources =

endif

EXTRA_DIST = $(cdaudiosources)

lib_LTLIBRARIES = $(cdaudioltlibs)

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

AM_CPPFLAGS = @GTK_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
libinput_cdaudio_la_LDFLAGS = @PLUGIN_LDFLAGS@
libinput_cdaudio_la_LIBADD = @GTK_LIBS@
libinput_cdaudio_la_SOURCES = $(cdaudiobuildsources)

