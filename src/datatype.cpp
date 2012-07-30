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
 * This class provides 'allocate' and 'free' functions needed to
 * keep track of certain functions like MPI_File_open and close. 
 * For example, 
 * On an MPI_File_open, allocate is called. 
 * On an MPI_File_close, free is called. 
 * At the end, if there are allocated objects that were not freed,
 * the callpaths and count is reported: you have MPI memory leaks!
 *
 * 'is_handle_null' needs to be defined before the class can be 
 * instantiated. This is necessary since it depends on the type of 
 * handle used (e.g., MPI_File). 
 */ 
static class MPI_Datatype2CallpathSet : public Handle2Set<MPI_Datatype>
{
public: 
  bool is_handle_null(MPI_Datatype handle) {
    return (handle == MPI_DATATYPE_NULL) ? 1 : 0; 
  }
} Datatype2Callpath; 



/************************************************
 * MPI datatype allocation calls 
 ************************************************/

int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_contiguous(count, oldtype, newtype); 
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, 
		    MPI_Datatype *newtype)
{
  int rc = PMPI_Type_vector(count, blocklength, stride, oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

/* deprecated, but still available starting with MPI-2.0 */
int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, 
		     MPI_Datatype *newtype)
{
  int rc = PMPI_Type_hvector(count, blocklength, stride, oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_indexed(int count, int *array_of_blocklengths, int *array_of_displacements, 
		     MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_indexed(count, array_of_blocklengths, array_of_displacements, 
			     oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

/* deprecated, but still available starting with MPI-2.0 */
int MPI_Type_hindexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, 
		      MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_hindexed(count, array_of_blocklengths, array_of_displacements, 
			      oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

/* deprecated, but still available starting with MPI-2.0 */
int MPI_Type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, 
		    MPI_Datatype *array_of_types, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_struct(count, array_of_blocklengths, array_of_displacements,
			    array_of_types, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride,
			    MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_hvector(count, blocklength, stride, oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_create_hindexed(int count, int array_of_blocklengths[], 
			     MPI_Aint array_of_displacements[], MPI_Datatype oldtype, 
			     MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_hindexed(count, array_of_blocklengths, array_of_displacements,
				     oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_create_indexed_block(int count, int blocklength, int array_of_displacements[], 
				  MPI_Datatype oldtype,  MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_indexed_block(count, blocklength, array_of_displacements, 
					  oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_create_struct(int count, int array_of_blocklengths[], 
			   MPI_Aint array_of_displacements[], MPI_Datatype array_of_types[], 
			   MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, 
				   array_of_types, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_create_subarray(int ndims, int array_of_sizes[], int array_of_subsizes[], 
			     int array_of_starts[], int order, MPI_Datatype oldtype, 
			     MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_subarray(ndims, array_of_sizes, array_of_subsizes, 
				     array_of_starts, order, oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_create_darray(int size, int rank, int ndims, int array_of_gsizes[], 
			   int array_of_distribs[], int array_of_dargs[], int array_of_psizes[], 
			   int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs,
				   array_of_dargs, array_of_psizes, order, oldtype, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, 
			    MPI_Datatype *newtype)
{
  int rc = PMPI_Type_create_resized(oldtype, lb, extent, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

int MPI_Type_dup(MPI_Datatype type, MPI_Datatype *newtype)
{
  int rc = PMPI_Type_dup(type, newtype);
  Datatype2Callpath.allocate(*newtype, chop); 
  return rc; 
}

#endif  

/************************************************
 * MPI datatype free call
 ************************************************/

int MPI_Type_free(MPI_Datatype *datatype)
{
  MPI_Datatype handle_copy = *datatype; 
  int rc = PMPI_Type_free(datatype); 
  Datatype2Callpath.free(handle_copy, chop); 
  return rc; 
  
}
