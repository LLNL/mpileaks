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
 * Track allocation and freeing of user-defined reduction ops.
 */ 
static class MPI_Errhandler2CallpathSet : public Handle2Set<MPI_Errhandler>
{
public: 
  bool is_handle_null(MPI_Errhandler handle) {
    return (handle == MPI_ERRHANDLER_NULL) ? 1 : 0; 
  }
} Errhandler2Callpath; 



/************************************************
 * MPI error handler allocation calls 
 ************************************************/

int MPI_Errhandler_create(MPI_Handler_function *function, MPI_Errhandler *eh)
{
  int rc = PMPI_Errhandler_create(function, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *eh)
{
  int rc = PMPI_Errhandler_get(comm, eh);
  Errhandler2Callpath.allocate(*eh, chop);
  return rc;
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Comm_create_errhandler(MPI_Comm_errhandler_function *function, MPI_Errhandler *eh)
{
  int rc = PMPI_Comm_create_errhandler(function, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *eh)
{
  int rc = PMPI_Comm_get_errhandler(comm, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

int MPI_Win_create_errhandler(MPI_Win_errhandler_function *function, MPI_Errhandler *eh)
{
  int rc = PMPI_Win_create_errhandler(function, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *eh)
{
  int rc = PMPI_Win_get_errhandler(win, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

int MPI_File_create_errhandler(MPI_File_errhandler_function *function, MPI_Errhandler *eh)
{
  int rc = PMPI_File_create_errhandler(function, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *eh)
{
  int rc = PMPI_File_get_errhandler(file, eh); 
  Errhandler2Callpath.allocate(*eh, chop); 
  return rc; 
}

#endif  

/************************************************
 * MPI error handler free call
 ************************************************/

int MPI_Errhandler_free(MPI_Errhandler *eh) 
{
  MPI_Errhandler handle_copy = *eh; 
  
  int rc = PMPI_Errhandler_free(eh);  
  Errhandler2Callpath.free(handle_copy, chop); 
  
  return rc; 
}
