/* Copyright (c) 2012, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Adam Moody <moody20@llnl.gov> and Edgar A. Leon <leon@llnl.gov>.
 * LLNL-CODE-557543.
 * All rights reserved.
 * This file is part of the mpileaks tool package.
 * For details, see https://github.com/hpc/mpileaks
 * Please also read this file: LICENSE.TXT. */

#include "mpi.h"
#include "mpileaks.h"


/*
 * Track allocation and freeing of attributes.  We track attributes
 * for comms,windows, and files separately, but all within this file.
 */ 
static class MPI_Keyval2CallpathSet : public Handle2Set<int>
{
public: 
  bool is_handle_null(int handle) {
    return (handle == MPI_KEYVAL_INVALID) ? 1 : 0; 
  }
} Commkeyval2Callpath, Winkeyval2Callpath, Typekeyval2Callpath; 


/************************************************
 * MPI error handler allocation calls 
 ************************************************/

/* deprecated with MPI-2.0, only applies to comms */
int MPI_Keyval_create(
  MPI_Copy_function *copy_fn,
  MPI_Delete_function *delete_fn,
  int *keyval,
  void* extra_state)
{
  int rc = PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state); 
  Commkeyval2Callpath.allocate(*keyval); 
  return rc; 
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Comm_create_keyval(
  MPI_Comm_copy_attr_function *comm_copy_attr_fn,
  MPI_Comm_delete_attr_function *comm_delete_attr_fn,
  int *comm_keyval,
  void *extra_state)
{
  int rc = PMPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state); 
  Commkeyval2Callpath.allocate(*comm_keyval); 
  return rc; 
}

int MPI_Win_create_keyval(
  MPI_Win_copy_attr_function *win_copy_attr_fn,
  MPI_Win_delete_attr_function *win_delete_attr_fn,
  int *win_keyval,
  void *extra_state)
{
  int rc = PMPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
  Winkeyval2Callpath.allocate(*win_keyval); 
  return rc; 
}

int MPI_Type_create_keyval(
  MPI_Type_copy_attr_function *type_copy_attr_fn,
  MPI_Type_delete_attr_function *type_delete_attr_fn,
  int *type_keyval,
  void *extra_state)
{
  int rc = PMPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state); 
  Typekeyval2Callpath.allocate(*type_keyval); 
  return rc; 
}

#endif  

/************************************************
 * MPI error handler free call
 ************************************************/

/* deprecated with MPI-2.0, only applies to comms */
int MPI_Keyval_free(int *keyval)
{
  int handle_copy = *keyval; 
  
  int rc = PMPI_Keyval_free(keyval);  
  Commkeyval2Callpath.free(handle_copy); 
  
  return rc; 
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Comm_free_keyval(int *keyval)
{
  int handle_copy = *keyval; 
  
  int rc = PMPI_Comm_free_keyval(keyval);  
  Commkeyval2Callpath.free(handle_copy); 
  
  return rc; 
}

int MPI_Win_free_keyval(int *keyval)
{
  int handle_copy = *keyval; 
  
  int rc = PMPI_Win_free_keyval(keyval);  
  Winkeyval2Callpath.free(handle_copy); 
  
  return rc; 
}

int MPI_Type_free_keyval(int *keyval)
{
  int handle_copy = *keyval; 
  
  int rc = PMPI_Type_free_keyval(keyval);  
  Typekeyval2Callpath.free(handle_copy); 
  
  return rc; 
}

#endif  
