##*****************************************************************************
#  AUTHOR:
#    Adam Moody <moody20@llnl.gov>
#
#  SYNOPSIS:
#    X_AC_ADEPTUTILS()
#
#  DESCRIPTION:
#    Check the usual suspects for a Callpath library installation,
#    setting ADEPTUTILS_CFLAGS, ADEPTUTILS_LDFLAGS, and ADEPTUTILS_LIBS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_ADEPTUTILS], [

  _x_ac_adeptutils_dirs="/usr /opt/freeware"
  _x_ac_adeptutils_libs="lib64 lib"

  AC_ARG_WITH(
    [adept-utils],
    AS_HELP_STRING(--with-adept-utils=PATH,Specify adept-utils path),
    [_x_ac_adeptutils_dirs="$withval"
     with_adeptutils=yes],
    [_x_ac_adeptutils_dirs=no
     with_adeptutils=no]
  )

  if test "x$_x_ac_adeptutils_dirs" = xno ; then
    # user explicitly wants to disable adept utils support
    ADEPTUTILS_CFLAGS=""
    ADEPTUTILS_LDFLAGS=""
    ADEPTUTILS_LIBS=""
    # TODO: would be nice to print some message here to record in the config log
  else
    # user wants adept utils enabled, so let's define it in the source code
    AC_DEFINE([HAVE_LIBADEPTUTILS], [1], [Define if adept-utils is available])

    # now let's locate the install location
    found=no

    # check for adept-utils in a system default location if:
    #   --with-adept-utils or --without-adept-utils is not specified
    #   --with-adept-utils=yes is specified
    #   --with-adept-utils is specified
    if test "$with_adeptutils" = check || \
       test "x$_x_ac_adeptutils_dirs" = xyes || \
       test "x$_x_ac_adeptutils_dirs" = "x" ; then
#      AC_CHECK_LIB([adept_utils], [GCS_Comm_split])

      # if we found it, set the build flags
      if test "$ac_cv_lib_adeptutils_GCS_Comm_split" = yes; then
        found=yes
        ADEPTUTILS_CFLAGS=""
        ADEPTUTILS_LDFLAGS=""
        ADEPTUTILS_LIBS="-ladept_cutils -ladept_timing -ladept_utils"
      fi
    fi

    # if we have not already found it, check the adeptutils_dirs
    # we look for a reasonable number of headers to be sure we found it
    if test "$found" = no; then
      AC_CACHE_CHECK(
        [for adept-utils installation],
        [x_ac_cv_adeptutils_dir],
        [
          for d in $_x_ac_adeptutils_dirs; do
            test -d "$d" || continue
            test -d "$d/include" || continue
            test -f "$d/include/io_utils.h" || continue
            test -f "$d/include/link_utils.h" || continue
            test -f "$d/include/matrix_utils.h" || continue
            test -f "$d/include/mpi_utils.h" || continue
            test -f "$d/include/string_utils.h" || continue
            for bit in $_x_ac_adeptutils_libs; do
              test -d "$d/$bit" || continue
        
# TODO: replace line below with link test
              x_ac_cv_adeptutils_dir=$d
#              _x_ac_adeptutils_libs_save="$LIBS"
#              LIBS="-L$d/$bit -ladept_utils $LIBS $MPI_CLDFLAGS"
#              AC_LINK_IFELSE(
#                AC_LANG_CALL([], [ADEPTUTILS_Comm_split]),
#                AS_VAR_SET([x_ac_cv_adeptutils_dir], [$d]))
#              LIBS="$_x_ac_adeptutils_libs_save"
#              test -n "$x_ac_cv_adeptutils_dir" && break
            done
            test -n "$x_ac_cv_adeptutils_dir" && break
          done
      ])

      # if we found it, set the build flags
      if test -n "$x_ac_cv_adeptutils_dir"; then
        found=yes
        ADEPTUTILS_CFLAGS="-I$x_ac_cv_adeptutils_dir/include"
        ADEPTUTILS_LDFLAGS="-L$x_ac_cv_adeptutils_dir/$bit"
        ADEPTUTILS_LIBS="-ladept_cutils -ladept_timing -ladept_utils"
      fi
    fi

    # if we failed to find adept utils, throw an error
    if test "$found" = no ; then
      AC_MSG_ERROR([unable to locate adept utils installation])
    fi
  fi

  # adept utils is required
  if test "x$ADEPTUTILS_LIBS" = "x" ; then
    AC_MSG_ERROR([unable to locate adept-utils installation])
  fi

  # propogate the build flags to our makefiles
  AC_SUBST(ADEPTUTILS_CFLAGS)
  AC_SUBST(ADEPTUTILS_LDFLAGS)
  AC_SUBST(ADEPTUTILS_LIBS)
])
