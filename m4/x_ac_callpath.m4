##*****************************************************************************
#  AUTHOR:
#    Adam Moody <moody20@llnl.gov>
#
#  SYNOPSIS:
#    X_AC_CALLPATH()
#
#  DESCRIPTION:
#    Check the usual suspects for a Callpath library installation,
#    setting CALLPATH_CFLAGS, CALLPATH_LDFLAGS, and CALLPATH_LIBS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_CALLPATH], [

  _x_ac_callpath_dirs="/usr /opt/freeware"
  _x_ac_callpath_libs="lib64 lib"

  AC_ARG_WITH(
    [callpath],
    AS_HELP_STRING(--with-callpath=PATH,Specify libcallpath path),
    [_x_ac_callpath_dirs="$withval"
     with_callpath=yes],
    [_x_ac_callpath_dirs=no
     with_callpath=no]
  )

  if test "x$_x_ac_callpath_dirs" = xno ; then
    # user explicitly wants to disable libcallpath support
    CALLPATH_CFLAGS=""
    CALLPATH_LDFLAGS=""
    CALLPATH_LIBS=""
    # TODO: would be nice to print some message here to record in the config log
  else
    # user wants libcallpath enabled, so let's define it in the source code
    AC_DEFINE([HAVE_LIBCALLPATH], [1], [Define if libcallpath is available])

    # now let's locate the install location
    found=no

    # check for libcallpath in a system default location if:
    #   --with-callpath or --without-callpath is not specified
    #   --with-callpath=yes is specified
    #   --with-callpath is specified
    if test "$with_callpath" = check || \
       test "x$_x_ac_callpath_dirs" = xyes || \
       test "x$_x_ac_callpath_dirs" = "x" ; then
#      AC_CHECK_LIB([callpath], [GCS_Comm_split])

      # if we found it, set the build flags
      if test "$ac_cv_lib_callpath_GCS_Comm_split" = yes; then
        found=yes
        CALLPATH_CFLAGS=""
        CALLPATH_LDFLAGS=""
        CALLPATH_LIBS="-lcallpath"
      fi
    fi

    # if we have not already found it, check the callpath_dirs
    if test "$found" = no; then
      AC_CACHE_CHECK(
        [for libcallpath installation],
        [x_ac_cv_callpath_dir],
        [
          for d in $_x_ac_callpath_dirs; do
            test -d "$d" || continue
            test -d "$d/include" || continue
            test -f "$d/include/Callpath.h" || continue
            for bit in $_x_ac_callpath_libs; do
              test -d "$d/$bit" || continue
        
# TODO: replace line below with link test
              x_ac_cv_callpath_dir=$d
#              _x_ac_callpath_libs_save="$LIBS"
#              LIBS="-L$d/$bit -lcallpath $LIBS $MPI_CLDFLAGS"
#              AC_LINK_IFELSE(
#                AC_LANG_CALL([], [CALLPATH_Comm_split]),
#                AS_VAR_SET([x_ac_cv_callpath_dir], [$d]))
#              LIBS="$_x_ac_callpath_libs_save"
#              test -n "$x_ac_cv_callpath_dir" && break
            done
            test -n "$x_ac_cv_callpath_dir" && break
          done
      ])

      # if we found it, set the build flags
      if test -n "$x_ac_cv_callpath_dir"; then
        found=yes
        CALLPATH_CFLAGS="-I$x_ac_cv_callpath_dir/include"
        CALLPATH_LDFLAGS="-L$x_ac_cv_callpath_dir/$bit"
        CALLPATH_LIBS="-lcallpath"
      fi
    fi

    # if we failed to find libcallpath, throw an error
    if test "$found" = no ; then
      AC_MSG_ERROR([unable to locate libcallpath installation])
    fi
  fi

  # callpath is required
  if test "x$CALLPATH_LIBS" = "x" ; then
    AC_MSG_ERROR([unable to locate libcallpath installation])
  fi

  # propogate the build flags to our makefiles
  AC_SUBST(CALLPATH_CFLAGS)
  AC_SUBST(CALLPATH_LDFLAGS)
  AC_SUBST(CALLPATH_LIBS)
])
