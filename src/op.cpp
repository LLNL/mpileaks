#include "mpi.h"
#include "mpileaks.h"


/*
 * Track allocation and freeing of user-defined reduction ops.
 */ 
static class MPI_Op2CallpathSet : public Handle2Set<MPI_Op>
{
public: 
  bool is_handle_null(MPI_Op handle) {
    return (handle == MPI_OP_NULL) ? 1 : 0; 
  }
} Op2Callpath; 



/************************************************
 * MPI calls 
 ************************************************/

int MPI_Op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
  int rc = PMPI_Op_create(function, commute, op); 
  Op2Callpath.allocate(*op); 
  return rc; 
}


int MPI_Op_free(MPI_Op *op) 
{
  MPI_Op handle_copy = *op; 
  
  int rc = PMPI_Op_free(op);  
  Op2Callpath.free(handle_copy); 
  
  return rc; 
}
