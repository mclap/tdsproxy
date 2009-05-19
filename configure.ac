#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(src/main.c)
AM_INIT_AUTOMAKE(tdsproxy, 0.1, pavel.plesov@gmail.com)
# AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h string.h sys/socket.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset socket])

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
