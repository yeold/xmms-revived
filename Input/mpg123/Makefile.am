lib_LTLIBRARIES = libmpg123.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)


COMMON_SRC =  mpg123.c configure.c fileinfo.c common.c \
decode_2to1.c decode_4to1.c decode_ntom.c \
layer1.c layer2.c layer3.c \
tabinit.c equalizer.c http.c \
huffman.h mpg123.h l2tables.h getbits.h \
dxhead.c dxhead.h \
id3.c id3.h id3_frame.c id3_frame_content.c id3_frame_text.c \
id3_frame_url.c id3_header.h id3_tag.c


if ARCH_X86

if USE_3DNOW

EXTRA_SRC = decode_i386.c decode_i586.s decode_3dnow.s dct64_i386.c dct64_3dnow.s dct36_3dnow.s getcpuflags.s
else

EXTRA_SRC = decode_i386.c decode_i586.s dct64_i386.c

endif

else

EXTRA_SRC = decode.c dct64.c

endif

EXTRA_DIST = decode_i386.c decode_i586.s dct64_i386.c getbits.c decode.c dct64.c decode_3dnow.s dct64_3dnow.s dct36_3dnow.s getcpuflags.s

libmpg123_la_SOURCES = $(COMMON_SRC) $(EXTRA_SRC)
libmpg123_la_LDFLAGS = @PLUGIN_LDFLAGS@
libmpg123_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ @Z_LIBS@
INCLUDES = @GTK_CFLAGS@ @ARCH_DEFINES@ -I$(top_builddir)/intl
