##*****************************************************************************
#  AUTHOR:
#    Adam Moody <moody20@llnl.gov>
#
#  SYNOPSIS:
#    X_AC_MPILEAKS()
#
#  DESCRIPTION:
#    Allows user to configure start and depth values for srun scripts.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_MPILEAKS], [

  AC_ARG_WITH(
    [stack-start-c],
    AS_HELP_STRING(--with-stack-start-c=value,Specify integer number for MPILEAKS_STACK_START for C code),
    [_x_ac_stack_start_c="$withval"
     with_stack_start_c=yes],
    [_x_ac_stack_start_c=2
     with_stack_start_c=no]
  )
  AC_ARG_WITH(
    [stack-start-fortran],
    AS_HELP_STRING(--with-stack-start-fortran=value,Specify integer number for MPILEAKS_STACK_START for FORTRAN code),
    [_x_ac_stack_start_fortran="$withval"
     with_stack_start_fortran=yes],
    [_x_ac_stack_start_fortran=3
     with_stack_start_fortran=no]
  )

  X_MPILEAKS_STACK_START_C=$_x_ac_stack_start_c
  X_MPILEAKS_STACK_START_FORTRAN=$_x_ac_stack_start_fortran

  # propogate the build flags to our makefiles
  AC_SUBST(X_MPILEAKS_STACK_START_C)
  AC_SUBST(X_MPILEAKS_STACK_START_FORTRAN)
])
