if HAVE_ESD

esdltlibs = liboutput_esd.la
esdsources = esd.c esdout.h \
mixer.c \
about.c \
configure.c \
audio.c \
init.c

else

esdltlibs =
esdsources =

endif

EXTRA_DIST = esd.c esdout.h mixer.c about.c configure.c audio.c init.c

libdir = $(plugindir)/$(OUTPUT_PLUGIN_DIR)
lib_LTLIBRARIES = $(esdltlibs)

AM_CPPFLAGS = @GTK_CFLAGS@ @ESD_CFLAGS@ -I$(top_builddir)/intl -I$(top_srcdir)
liboutput_esd_la_LDFLAGS = @PLUGIN_LDFLAGS@
liboutput_esd_la_LIBADD = @GTK_LIBS@ @ESD_LIBS@

liboutput_esd_la_SOURCES= $(esdsources)
