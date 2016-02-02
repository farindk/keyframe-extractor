#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([extractor], [0.2], [dirk.farin@gmail.com])
AC_CONFIG_SRCDIR([src/extractor.cc])
AC_CONFIG_HEADERS([config.h])

NUMERIC_VERSION=0x00010000 # Numeric representation of the version
AC_SUBST(NUMERIC_VERSION)

# ---------------------------------------------------------------------------

AC_CANONICAL_SYSTEM

dnl Initialize libtool
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_GREP

# Initialize automake stuff
AM_INIT_AUTOMAKE

CFLAGS+=" -std=c99"
CXXFLAGS+=" -Wreturn-type"
AX_CXX_COMPILE_STDCXX_11()

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h malloc.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([malloc memmove memset __malloc_hook memalign posix_memalign __mingw_aligned_malloc __mingw_aligned_free])

AC_SEARCH_LIBS([sqrt], [m])
AC_SEARCH_LIBS([pthread_create], [pthread])

AM_CONDITIONAL(MINGW, expr $host : '.*-mingw' >/dev/null 2>&1)


PKG_CHECK_MODULES([VIDEOGFX], [libvideogfx],
                  [AC_DEFINE([HAVE_VIDEOGFX], [1], [Whether libvideogfx was found.])
                   AC_SUBST(VIDEOGFX_CFLAGS)
                   AC_SUBST(VIDEOGFX_LIBS)
                   have_videogfx="yes"],
                  [have_videogfx="no"]
                  )



# --- ffmpeg ---

PKG_CHECK_MODULES([SWSCALE], [libswscale],
                     [AC_DEFINE([HAVE_SWSCALE], [1], [Whether libswscale was found.])
                      AC_SUBST(SWSCALE_CFLAGS)
                      AC_SUBST(SWSCALE_LIBS)
                      have_swscale="yes"],
                     [have_swscale="no"]
                     )
PKG_CHECK_MODULES([AVUTIL], [libavutil],
                     [AC_DEFINE([HAVE_AVUTIL], [1], [Whether libavutil was found.])
                      AC_SUBST(AVUTIL_CFLAGS)
                      AC_SUBST(AVUTIL_LIBS)
                      have_avutil="yes"],
                     [have_avutil="no"]
                     )
PKG_CHECK_MODULES([AVFORMAT], [libavformat],
                     [AC_DEFINE([HAVE_AVFORMAT], [1], [Whether libavformat was found.])
                      AC_SUBST(AVFORMAT_CFLAGS)
                      AC_SUBST(AVFORMAT_LIBS)
                      have_avformat="yes"],
                     [have_avformat="no"]
                     )
PKG_CHECK_MODULES([AVCODEC], [libavcodec],
                     [AC_DEFINE([HAVE_AVCODEC], [1], [Whether libavcodec was found.])
                      AC_SUBST(AVCODEC_CFLAGS)
                      AC_SUBST(AVCODEC_LIBS)
                      have_avcodec="yes"],
                     [have_avcodec="no"]
                     )

#AC_CHECK_HEADERS([libavcodec/avcodec.h ffmpeg/avcodec.h])
#AC_CHECK_HEADERS([libavformat/avformat.h ffmpeg/avformat.h])
#AC_CHECK_HEADERS([libswscale/swscale.h ffmpeg/swscale.h])
#AC_CHECK_HEADERS([libavutil/avutil.h ffmpeg/avutil.h])



# --- x264 ---  (TODO: why do we need this? isn't ffmpeg pulling this in itself?)

PKG_CHECK_MODULES([X264], [x264],
                     [AC_DEFINE([HAVE_X264], [1], [Whether libx264 was found.])
                      AC_SUBST(X264_CFLAGS)
                      AC_SUBST(X264_LIBS)
                      have_x264="yes"],
                     [have_x264="no"]
                     )



# --- output configuration results ---

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT
