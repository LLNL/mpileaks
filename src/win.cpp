#include "mpi.h"
#include "mpileaks.h"                 /* Handle2Callpath */ 


#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

/*
 * This class tracks window objects.
 */ 
static class MPI_Win2Callpath : public Handle2Callpath<MPI_Win>
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
