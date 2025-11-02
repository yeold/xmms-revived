dnl Process this file with autoconf to produce a configure script.

AC_INIT
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([configfile.c])

AM_CONFIG_HEADER([config.h])

dnl libtool version:
LIBXMMS_MAJOR_VERSION=4
LIBXMMS_MINOR_VERSION=1
LIBXMMS_MICRO_VERSION=3
LIBXMMS_VERSION=$LIBXMMS_MAJOR_VERSION.$LIBXMMS_MINOR_VERSION.$LIBXMMS_MICRO_VERSION

AM_INIT_AUTOMAKE([libxmms], [$LIBXMMS_VERSION])

AC_SUBST([LIBXMMS_MAJOR_VERSION])
AC_SUBST([LIBXMMS_MINOR_VERSION])
AC_SUBST([LIBXMMS_MICRO_VERSION])

AC_PREFIX_PROGRAM([xmms])
dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

LT_INIT
AM_ENABLE_STATIC
AM_ENABLE_SHARED

AC_C_BIGENDIAN

AC_CHECK_FUNC([sched_setscheduler])
AC_CHECK_FUNC([nanosleep])

# Check for POSIX libraries
AC_SEARCH_LIBS([clock_gettime], [rt])
POSIX_LIBS="$LIBS"
AC_SUBST(POSIX_LIBS)
LIBS=""

AM_PATH_GLIB_2_0([2.0.0],,[AC_MSG_ERROR([*** GLIB >= 2.0.0 not installed - please install first ***])])
AM_PATH_GTK_2_0([2.0.0],,[AC_MSG_ERROR([*** GTK+ >= 2.0.0 not installed - please install first ***])],[gthread])

if test -n "$GCC"
then
  CFLAGS="$CFLAGS -Wall -Wpointer-arith"
fi

HAVE_SYS_TIME_H=$ac_cv_header_sys_time_h
# Obsolete code to be removed.
if test $ac_cv_header_sys_time_h = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],[Define to 1 if you can safely include both <sys/time.h>
	     and <time.h>.  This macro is obsolete.])
fi
# End of obsolete code.


AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([sys/sched.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/time.h])

AC_CHECK_FUNCS([iconv])
AC_CHECK_HEADERS([iconv.h])
AC_ARG_WITH([libiconv-prefix],
  [AS_HELP_STRING([--with-libiconv-prefix=DIR], [search for libiconv in DIR/include and DIR/lib])],
  [for dir in `echo "$withval" | tr : ' '`; do
    if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
    if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
   done])
AC_CHECK_LIB([iconv], [libiconv_open], [LIBS="$LIBS -liconv"])

AC_CACHE_CHECK([for nl_langinfo(CODESET)], [xmms_cv_func_langinfo], [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <langinfo.h>]], [[char *codeset = nl_langinfo (CODESET);]])],[xmms_cv_func_langinfo=yes],[xmms_cv_func_langinfo=no])])
if test "x$xmms_cv_func_langinfo" = "xyes"; then
  AC_DEFINE([HAVE_CODESET],,[Define if nl_langinfo(CODESET) is available.])
fi


AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
