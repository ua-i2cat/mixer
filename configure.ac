#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([rtpMixer], [0.1], [BUG-REPORT-ADDRESS])

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects],[controller], [0.1])
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lavcodec':
AC_CHECK_LIB([avcodec], [av_fast_malloc])
# FIXME: Replace `main' with a function in `-lavformat':
AC_CHECK_LIB([avformat], [avformat_version])
# FIXME: Replace `main' with a function in `-liomanager':
AC_CHECK_LIB([iomanager], [init_decoder])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [pthread_create])
# FIXME: Replace `main' with a function in `-lrtp':
AC_CHECK_LIB([rtp], [next_csrc])
# FIXME: Replace `main' with a function in `-lswscale':
AC_CHECK_LIB([swscale], [sws_scale])
# FIXME: Replace `main' with a function in `-lvcompress':
AC_CHECK_LIB([vcompress], [parse_fmt])
# FIXME: Replace `main' with a function in `-lvdecompress':
AC_CHECK_LIB([vdecompress], [libavcodec_decompress_reconfigure])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor gettimeofday])

AC_OUTPUT([Makefile libmixer/Makefile])
