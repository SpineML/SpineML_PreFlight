dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([spineml_preflight], [0.1], [seb.james@sheffield.ac.uk])
AC_CONFIG_SRCDIR(connection_list.h)
AC_REVISION([$Revision: 1.0.0 $])
AC_PREFIX_DEFAULT(/usr/local)

AM_INIT_AUTOMAKE([1.9 foreign])

AC_CONFIG_HEADER(config.h)

AC_LANG_CPLUSPLUS

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
dnl We don't ever use static versions of the components so disable static:
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl check for libs
dnl AC_CHECK_LIB([futil], [main])

dnl Checks for header files. Add more here...
AC_CHECK_HEADERS([sys/types.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_PID_T

dnl Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_CHECK_FUNCS([memset mkdir]) # would be more of these.

dnl Checks for endianness
AC_C_BIGENDIAN

dnl
dnl Checks for word sizes
dnl

AC_CHECK_SIZEOF([char])
AC_CHECK_SIZEOF([unsigned char])
if test "$ac_cv_sizeof_char" -eq 1; then
  AC_DEFINE(INT8_TYPE, [char], [Signed 8 bit])
else
  AC_MSG_ERROR([compiler has no 1 byte char])
fi
if test "$ac_cv_sizeof_unsigned_char" -eq 1; then
  AC_DEFINE(UINT8_TYPE, [unsigned char], [Unsigned 8 bit])
else
  AC_MSG_ERROR([compiler has no 1 byte unsigned char])
fi

AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([unsigned short])
if test "$ac_cv_sizeof_short" -eq 2; then
  AC_DEFINE(INT16_TYPE, [short], [Signed 16 bit])
else
  AC_MSG_ERROR([compiler has no 2 byte short])
fi
if test "$ac_cv_sizeof_unsigned_short" -eq 2; then
  AC_DEFINE(UINT16_TYPE, [unsigned short], [Unsigned 16 bit])
else
  AC_MSG_ERROR([compiler has no 2 byte unsigned short])
fi

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([unsigned int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([unsigned long])
if test "$ac_cv_sizeof_int" -eq 4; then
  AC_DEFINE(INT32_TYPE, [int], [Signed 32 bit])
elif test "$ac_cv_sizeof_long" -eq 4; then
  AC_DEFINE(INT32_TYPE, [long], [Signed 32 bit])
else
  AC_MSG_ERROR([compiler has no 4 byte integer])
fi
if test "$ac_cv_sizeof_unsigned_int" -eq 4; then
  AC_DEFINE(UINT32_TYPE, [unsigned int], [Unsigned 32 bit])
elif test "$ac_cv_sizeof_unsigned_long" -eq 4; then
  AC_DEFINE(UINT32_TYPE, [unsigned long], [Unsigned 32 bit])
else
  AC_MSG_ERROR([compiler has no 4 byte unsigned integer])
fi

if test "$ac_cv_sizeof_long" -eq 8; then
  AC_DEFINE(INT64_TYPE, [long], [Signed 64 bit])
else
  AC_CHECK_SIZEOF([long long])
  if test "$ac_cv_sizeof_long_long" -eq 8; then
    AC_DEFINE(HAVE_LONG_LONG, 1, [Have the long long type])
    AC_DEFINE(INT64_TYPE, [long long], [Signed 64 bit])
  else
    AC_MSG_ERROR([compiler has no 8 byte integer])
  fi
fi
if test "$ac_cv_sizeof_unsigned_long" -eq 8; then
  AC_DEFINE(UINT64_TYPE, [unsigned long], [Unsigned 32 bit])
else
  AC_CHECK_SIZEOF([unsigned long long])
  if test "$ac_cv_sizeof_unsigned_long_long" -eq 8; then
    AC_DEFINE(HAVE_UNSIGNED_LONG_LONG, 1, [Have the unsigned long long type])
    AC_DEFINE(UINT64_TYPE, [unsigned long long], [Unsigned 32 bit])
  else
    AC_MSG_ERROR([compiler has no 8 byte unsigned integer])
  fi
fi

AC_CHECK_SIZEOF([float])

if test "$ac_cv_sizeof_float" -eq 4; then
  AC_DEFINE(FLOAT4_TYPE, [float], [32 bit float])
else
  AC_MSG_ERROR([compiler has no 4 byte float])
fi

AC_CHECK_SIZEOF([double])

if test "$ac_cv_sizeof_double" -eq 8; then
  AC_DEFINE(DOUBLE8_TYPE, [double], [64 bit float])
else
  AC_MSG_ERROR([compiler has no 8 byte double])
fi

dnl Check for popt - perhaps only if user asks for a check? Iceberg
dnl has popt, but not the popt.pc file.
PKG_CHECK_MODULES(POPT, popt >= 1.0, [
                        AC_MSG_NOTICE([Found popt using pkg-config :)])
                        ], [
                        AC_MSG_WARN([Failed to find popt with pkg-config. Carrying on in the hope that it's there anyway...])
                        ])

dnl List the Makefiles to output
AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
