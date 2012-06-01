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


#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

/*
 * This class tracks window objects.
 */ 
static class MPI_Win2CallpathSet : public Handle2Set<MPI_Win>
{
public: 
  bool is_handle_null(MPI_Win handle) {
    return (handle == MPI_WIN_NULL) ? 1 : 0; 
  }
} Win2Callpath; 



/************************************************
 * MPI calls 
 ************************************************/

int MPI_Win_create(void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
  int rc = PMPI_Win_create(base, size, disp_unit, info, comm, win);
  Win2Callpath.allocate(*win); 
  return rc; 
}

int MPI_Win_free(MPI_Win *win) 
{
  MPI_Win handle_copy = *win; 
  
  int rc = PMPI_Win_free(win);  
  Win2Callpath.free(handle_copy); 
  
  return rc; 
}

#endif
