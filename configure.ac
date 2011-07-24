# Copyright (C) 2011 John Ferlito (johnf@inodes.org)

# Require autoconf version
AC_PREREQ([2.67])

AC_INIT([m3u8-segmenter], [0.3],
        [http://github.com/johnf/m3u8-segmenter/issues],
        [m3u8-segmenter],
        [http://m3u8-segmenter.inodes.org])

AC_CONFIG_SRCDIR([m3u8-segmenter.c])

# Put config stuff in Cfg.
AC_CONFIG_AUX_DIR(cfg)

AM_INIT_AUTOMAKE($PACKAGE_NAME,$PACKAGE_VERSION)

AC_LANG_C

dnl find and test the C compiler
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL


AC_HEADER_STDC
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([getopt.h])

AC_FUNC_MALLOC
AC_CHECK_FUNCS([memmove memset strrchr strtol])

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(FFMPEG, libavformat libavcodec libavutil)

AC_SUBST(FFMPEG_CFLAGS)
AC_SUBST(FFMPEG_LIBS)

AC_SUBST(PACKAGE_VERSION)

CFLAGS="$CFLAGS -Wall -Wextra -Wdeclaration-after-statement -Wpointer-arith -funsigned-char -Werror"
CFLAGS="$CFLAGS -Wall -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return -Wcast-align -Wcast-qual -Wnested-externs -Wshadow -Wbad-function-cast -Wwrite-strings "


AC_CONFIG_FILES([Makefile tests/Makefile])
AC_OUTPUT
