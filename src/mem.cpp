#include "mpi.h"
#include "mpileaks.h"                 /* Handle2Callpath */ 


#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

/*
 * This class tracks memory allocated and freed via calls to MPI.
 */ 
static class MPI_Mem2Callpath : public Handle2Callpath<void*>
{
public: 
  bool is_handle_null(void* handle) {
    /* memory is always valid, an error is thrown at allocation if invalid
     * and the "handle" is not changed to NULL on free */
    return 0;
  }
} Mem2Callpath; 



/************************************************
 * MPI calls 
 ************************************************/

int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
  int rc = PMPI_Alloc_mem(size, info, baseptr); 
  Mem2Callpath.allocate(*(void**)baseptr); 
  return rc; 
}


int MPI_Free_mem(void* base) 
{
  void* handle_copy = base; 
  
  int rc = PMPI_Free_mem(base);  
  Mem2Callpath.free(handle_copy); 
  
  return rc; 
}

#endif
