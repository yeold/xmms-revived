lib_LTLIBRARIES = libidcin.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)

INCLUDES = @GTK_CFLAGS@ -I$(top_builddir)/intl
libidcin_la_LDFLAGS = @PLUGIN_LDFLAGS@
libidcin_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@

libidcin_la_SOURCES = 	\
idcin.c \
idcin.h

