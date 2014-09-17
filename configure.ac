##
# Prologue.
##
AC_INIT([mpileaks],[1.0],[moody20@llnl.gov],[mpileaks],[https://github.com/hpc/mpileaks])
AC_CONFIG_SRCDIR([src/mpileaks.cpp])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER([config/config.h])

#AC_CANONICAL_SYSTEM

X_AC_META
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support.
##
AM_INIT_AUTOMAKE([foreign])

#AM_MAINTAINER_MODE

##
# Checks for programs.
##
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
#X_AC_PROG_MPICC
LX_FIND_MPI
X_AC_ADEPTUTILS
X_AC_CALLPATH
X_AC_MPILEAKS

##
# enable libtool
##
AC_PROG_LIBTOOL

##
# Extra packages
##

##
# Checks for header files.
##
AC_HEADER_STDC

##
# Checks for typedefs, structures, and compiler characteristics.
##
AC_C_BIGENDIAN

##
# Checks for library functions.
##

##
# Epilogue.
##
AC_CONFIG_FILES([ \
  Makefile \
  src/Makefile \
  examples/Makefile \
  scripts/Makefile \
  scripts/srun-mpileaks \
  scripts/srun-mpileaksf \
])
AC_OUTPUT

