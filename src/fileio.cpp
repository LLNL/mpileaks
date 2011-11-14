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
static class MPI_File2CallpathSet : public Handle2Set<MPI_File>
{
public: 
  bool is_handle_null(MPI_File handle) {
    return (handle == MPI_FILE_NULL) ? 1 : 0; 
  }
} File2Callpath; 



/************************************************
 * MPI calls 
 ************************************************/

int MPI_File_open(MPI_Comm comm, char *filename, int amode, MPI_Info info, MPI_File *fh)
{
  int rc = PMPI_File_open(comm, filename, amode, info, fh); 
  File2Callpath.allocate(*fh); 
  return rc; 
}


int MPI_File_close(MPI_File *fh) 
{
  MPI_File handle_copy = *fh; 
  
  int rc = PMPI_File_close(fh);  
  File2Callpath.free(handle_copy); 
  
  return rc; 
}
