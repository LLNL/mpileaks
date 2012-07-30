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
 * This class tracks info objects.
 */ 
static class MPI_Info2CallpathSet : public Handle2Set<MPI_Info>
{
public: 
  bool is_handle_null(MPI_Info handle) {
    return (handle == MPI_INFO_NULL) ? 1 : 0; 
  }
} Info2Callpath; 



/************************************************
 * MPI calls 
 ************************************************/

int MPI_Info_create(MPI_Info *info)
{
  int rc = PMPI_Info_create(info);
  Info2Callpath.allocate(*info, chop); 
  return rc; 
}

int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
  int rc = PMPI_Info_dup(info, newinfo);
  Info2Callpath.allocate(*newinfo, chop); 
  return rc; 
}

int MPI_File_get_info(MPI_File fh, MPI_Info *info_used)
{
  int rc = PMPI_File_get_info(fh, info_used);
  Info2Callpath.allocate(*info_used, chop); 
  return rc; 
}

int MPI_Info_free(MPI_Info *info) 
{
  MPI_Info handle_copy = *info; 
  
  int rc = PMPI_Info_free(info);  
  Info2Callpath.free(handle_copy, chop); 
  
  return rc; 
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

#endif
