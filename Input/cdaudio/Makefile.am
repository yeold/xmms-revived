if HAVE_CDROM

cdaudioltlibs = libcdaudio.la
cdaudiosources = cdaudio.c \
configure.c \
cdaudio.h \
cdinfo.c \
cdinfo.h \
cddb.c \
cddb.h \
http.c \
http.h \
cdindex.c \
sha_func.c \
sha.h

else

cdaudioltlibs =
cdaudiosources =

endif

EXTRA_DIST = cdaudio.c configure.c cdaudio.h cdinfo.c cdinfo.h cddb.c cddb.h http.c http.h cdindex.c sha_func.c sha.h

lib_LTLIBRARIES = $(cdaudioltlibs)

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ @XML_CFLAGS@ -I$(top_builddir)/intl
libcdaudio_la_LDFLAGS = @PLUGIN_LDFLAGS@
libcdaudio_la_LIBADD = @GTK_LIBS@ @XML_LIBS@
libcdaudio_la_SOURCES = $(cdaudiosources)

