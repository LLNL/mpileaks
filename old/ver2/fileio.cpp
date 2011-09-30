#include "mpi.h"
#include "mpileaks.h"                 /* Handle2Callpath */ 


/************************************************
 * Required declarations
 ************************************************/

/*
 * An object of type Handle2Callpath instantiated to a given type 
 * (e.g., MPI_Request, MPI_File)
 */ 
static Handle2Callpath< MPI_File, Handle<MPI_File> > File2Callpath; 


/* 
 * A class of type Handle which defines certain functions that are
 * specific to the given type
 */ 
template<> class Handle<MPI_File> {
public: 
  static int isnull(MPI_File handle) {
    return (handle == MPI_FILE_NULL) ? 1 : 0; 
  }
}; 



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
