#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bin/take], [0.0.2], [tero.isannainen@gmail.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/take.c])
AC_CONFIG_HEADERS([src/config.h])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([como], [como_init], [], [])

AC_CHECK_LIB([termbox], [tb_init], [], [])

AC_CHECK_LIB([termbox], [tb_init],
             [AC_DEFINE([USE_TERMBOX], [1],
               [Define to 1 if you have termbox library (i.e. <termbox.h> etc).])],
             [AC_CHECK_LIB([ncurses], [initscr], [],
               [AC_MSG_ERROR([required library])])])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset regcomp strdup strtol])

AC_CHECK_FUNC(vfork, AC_DEFINE([HAVE_VFORK], [], [vfork function available]))

AC_ARG_ENABLE([mark-color], [  --enable-mark-color    enable the use of color for line marking])
AS_IF([test "x$enable_mark_color" = "xyes"], [
 AC_DEFINE([ENABLE_MARK_COLOR], [1], [Enable color marking.])
])

AC_OUTPUT(Makefile src/Makefile man/Makefile)
