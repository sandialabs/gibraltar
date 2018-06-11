#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gibraltar], [1.0], [mlcurry@sandia.gov])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([examples/sweeping_test.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([cuda], [cuInit])
AC_CHECK_LIB([jerasure], [jerasure_do_parity])

# Enable or disable jerasure or CUDA
AC_ARG_ENABLE([jerasure],
	AS_HELP_STRING([--enable-jerasure],
		[enable the jerasure API])
)

AS_IF([test "x$enable_jerasure" = "xyes"],
	dnl Do the stuff to enable Jerasure
])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h sys/time.h unistd.h cuda_runtime_api.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero getcwd gettimeofday memmove memset mkdir strchr strrchr])

# Add list of files to generate
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])

AC_OUTPUT
