#include "mpi.h"
#include "mpileaks.h"                 /* Handle2Callpath */ 

/*
 * This class provides 'allocate' and 'free' functions needed to
 * keep track of certain functions like MPI_ISend with MPI_Wait. 
 * For example, 
 * On an MPI_ISend, allocate is called. 
 * On an MPI_Wait, free is called. 
 * At the end, if there are allocated objects that were not freed,
 * the callpaths and count is reported: you have MPI memory leaks!
 *
 * 'is_handle_null' needs to be defined before the class can be 
 * instantiated. This is necessary since it depends on the type of 
 * handle used (e.g., MPI_Request). 
 */ 
static class MPI_Request2Callpath : public Handle2Callpath<MPI_Request>
{
public: 
  bool is_handle_null(MPI_Request handle) {
    return (handle == MPI_REQUEST_NULL) ? 1 : 0; 
  }
} Request2Callpath; 


/************************************************
 * Auxiliary functions
 ************************************************/

/* allocate memory, make a full copy of the specified request array, and return the copy */
static MPI_Request* mpileaks_request_copy_array(int count, MPI_Request req[])
{
  if (count <= 0) {
    return NULL;
  }

  MPI_Request* req_copies = (MPI_Request*) malloc(count * sizeof(MPI_Request));

  int i;
  for (i = 0; i < count; i++) {
    req_copies[i] = req[i];
  }

  return req_copies;
}

/* given a copy of the request array and a new version, free any requests that
 * have changed to MPI_REQUEST_NULL */
static void mpileaks_request_free_array(int count, MPI_Request req1[], MPI_Request req2[])
{
  int i;
  for (i = 0; i < count; i++) {
    if (req1[i] != MPI_REQUEST_NULL && req2[i] == MPI_REQUEST_NULL) {
      Request2Callpath.free(req1[i]);
    }
  }
}



/************************************************
 * Send calls 
 ************************************************/

int MPI_Isend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, 
	      MPI_Request* req)
{
  int rc = PMPI_Isend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req); 
  return rc;
}

int MPI_Issend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, 
	       MPI_Request* req)
{
  int rc = PMPI_Issend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req);
  return rc;
}

int MPI_Irsend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Irsend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req);
  return rc;
}

int MPI_Ibsend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Ibsend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req);
  return rc;
}

/************************************************
 * Receive calls 
 ************************************************/

int MPI_Irecv(void* buf, int count, MPI_Datatype dt, int src, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Irecv(buf, count, dt, src, tag, comm, req);
  Request2Callpath.allocate(*req);
  return rc;
}





/* other allocation calls */

/* TODO:
  MPI_Start
  MPI_Startall
  MPI_Send_init
  MPI_Recv_init
  Generalized requests
*/

/* free calls */

int MPI_Request_free(MPI_Request* req)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Request_free(req);

  if (req_copy != MPI_REQUEST_NULL) {
    Request2Callpath.free(req_copy);
  }

  return rc;
}

int MPI_Wait(MPI_Request* req, MPI_Status* stat)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Wait(req, stat);

  if (req_copy != MPI_REQUEST_NULL) {
    Request2Callpath.free(req_copy);
  }

  return rc;
}

int MPI_Test(MPI_Request* req, int* flag, MPI_Status* stat)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Test(req, flag, stat);

  if (*flag && req_copy != MPI_REQUEST_NULL) {
    Request2Callpath.free(req_copy);
  }

  return rc;
}

int MPI_Waitany(int count, MPI_Request req[], int* index, MPI_Status* stat)
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Waitany(count, req, index, stat);

  int idx = *index;
  if (idx != MPI_UNDEFINED) {
    Request2Callpath.free(req_copies[idx]);
  }
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}

int MPI_Testany(int count, MPI_Request req[], int* index, int* flag, MPI_Status* stat)
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Testany(count, req, index, flag, stat);

  int idx = *index;
  if (idx != MPI_UNDEFINED && *flag) {
    Request2Callpath.free(req_copies[idx]);
  }
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}

int MPI_Waitall(int count, MPI_Request req[], MPI_Status stat[])
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Waitall(count, req, stat);

  mpileaks_request_free_array(count, req_copies, req);
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}

int MPI_Testall(int count, MPI_Request req[], int* flag, MPI_Status stat[])
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Testall(count, req, flag, stat);

  /* we can't use the flag value here, since this may complete
   * some but not all requests */
  mpileaks_request_free_array(count, req_copies, req);
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}

int MPI_Waitsome(int count, MPI_Request req[], int* outcount, int indicies[], MPI_Status stat[])
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Waitsome(count, req, outcount, indicies, stat);

  if (*outcount != 0 && *outcount != MPI_UNDEFINED) {
    mpileaks_request_free_array(count, req_copies, req);
  }
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}

int MPI_Testsome(int count, MPI_Request req[], int* outcount, int indicies[], MPI_Status stat[])
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Testsome(count, req, outcount, indicies, stat);

  if (*outcount != 0 && *outcount != MPI_UNDEFINED) {
    mpileaks_request_free_array(count, req_copies, req);
  }
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}
