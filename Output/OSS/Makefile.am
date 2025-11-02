if HAVE_OSS

ossltlibs = libOSS.la
osssources = OSS.c OSS.h \
mixer.c \
about.c \
configure.c \
audio.c \
init.c \
convert.c

else

ossltlibs =
osssources =

endif

lib_LTLIBRARIES = $(ossltlibs)

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)

libOSS_la_LDFLAGS = @PLUGIN_LDFLAGS@
libOSS_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@
libOSS_la_SOURCES= $(osssources)
INCLUDES = @GTK_CFLAGS@ @OSS_CFLAGS@ -I$(top_builddir)/intl
