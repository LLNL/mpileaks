/* Copyright (c) 2012, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Adam Moody <moody20@llnl.gov> and Edgar A. Leon <leon@llnl.gov>.
 * LLNL-CODE-557543.
 * All rights reserved.
 * This file is part of the mpileaks tool package.
 * For details, see https://github.com/hpc/mpileaks
 * Please also read this file: LICENSE.TXT. */

#include "mpi.h"
#include "mpileaks.h"                 /* Handle2Set */ 

/*
 * This class provides 'allocate' and 'free' functions needed to
 * keep track of certain functions like MPI_ISend with MPI_Wait. 
 * For example, 
 * On an MPI_Send_init or MPI_Start, allocate is called. 
 * On an MPI_Wait or MPI_Request_free, free is called. 
 * At the end, if there are allocated objects that were not freed,
 * the callpaths and count is reported: you have MPI memory leaks!
 *
 * 'is_handle_null' needs to be defined before the class can be 
 * instantiated. This is necessary since it depends on the type of 
 * handle used (e.g., MPI_Request). 
 */ 
static class MPI_Request2CallpathSet : public Handle2Set<MPI_Request>
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

static void mpileaks_request_allocate_array(int count, MPI_Request *reqs, size_t start)
{
  int i; 
  for (i = 0; i < count; i++) {
    Request2Callpath.allocate(reqs[i], start+1); 
  }
}


/* given a copy of the request array and a new version, free any requests that
 * have changed to MPI_REQUEST_NULL */
static void mpileaks_request_free_array(int count, MPI_Request req1[], MPI_Request req2[], size_t start)
{
  int i;
  for (i = 0; i < count; i++) {
    if (req1[i] != MPI_REQUEST_NULL && req2[i] == MPI_REQUEST_NULL) {
      Request2Callpath.free(req1[i], start+1);
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
  Request2Callpath.allocate(*req, chop); 
  return rc;
}

int MPI_Issend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, 
	       MPI_Request* req)
{
  int rc = PMPI_Issend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req, chop);
  return rc;
}

int MPI_Irsend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Irsend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req, chop);
  return rc;
}

int MPI_Ibsend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Ibsend(buf, count, dt, dest, tag, comm, req);
  Request2Callpath.allocate(*req, chop);
  return rc;
}

/************************************************
 * Receive calls 
 ************************************************/

int MPI_Irecv(void* buf, int count, MPI_Datatype dt, int src, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Irecv(buf, count, dt, src, tag, comm, req);
  Request2Callpath.allocate(*req, chop);
  return rc;
}


/************************************************
 * Persistent communication
 * An 'init' call should match an MPI_Request_free. 
 * A 'start' call should match a (multiple) complete operation (e.g., MPI_Wait). 
 ************************************************/

int MPI_Send_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		  MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request); 
  Request2Callpath.allocate(*request, chop); 
  return rc; 
}

int MPI_Bsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		   MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request); 
  Request2Callpath.allocate(*request, chop); 
  return rc; 
}

int MPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		   MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request); 
  Request2Callpath.allocate(*request, chop); 
  return rc; 
}

int MPI_Rsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		   MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request); 
  Request2Callpath.allocate(*request, chop); 
  return rc; 
}

int MPI_Recv_init(void* buf, int count, MPI_Datatype datatype, int source, int tag, 
		  MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Recv_init(buf, count, datatype, source, tag, comm, request); 
  Request2Callpath.allocate(*request, chop); 
  return rc; 
}

int MPI_Start(MPI_Request *request)
{
  int rc = PMPI_Start(request); 
  Request2Callpath.allocate(*request, chop); 
  return rc; 
}

int MPI_Startall(int count, MPI_Request *array_of_requests)
{
  int rc = PMPI_Startall(count, array_of_requests); 
  mpileaks_request_allocate_array(count, array_of_requests, chop); 
  return rc; 
}

#if MPI_VERSION > 1

/************************************************
 * File I/O calls
 ************************************************/

int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                      MPI_Datatype datatype, MPI_Request *request)
{
  int rc = PMPI_File_iread_at(fh, offset, buf, count, datatype, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Request *request)
{
  int rc = PMPI_File_iwrite_at(fh, offset, buf, count, datatype, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

int MPI_File_iread(MPI_File fh, void *buf, int count,
                   MPI_Datatype datatype, MPI_Request *request)
{
  int rc = PMPI_File_iread(fh, buf, count, datatype, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

int MPI_File_iwrite(MPI_File fh, void *buf, int count,
                   MPI_Datatype datatype, MPI_Request *request)
{
  int rc = PMPI_File_iwrite(fh, buf, count, datatype, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

int MPI_File_iread_shared(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Request *request)
{
  int rc = PMPI_File_iread_shared(fh, buf, count, datatype, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

int MPI_File_iwrite_shared(MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Request *request)
{
  int rc = PMPI_File_iwrite_shared(fh, buf, count, datatype, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

/* This function allocates a new request object that the user must eventually free
 * in a call to MPI_{WAIT,TEST}{ANY,SOME,ALL} or MPI_REQUEST_FREE */
int MPI_Grequest_start(
  MPI_Grequest_query_function  *query_fn,
  MPI_Grequest_free_function   *free_fn,
  MPI_Grequest_cancel_function *cancel_fn,
  void *extra_state, MPI_Request *request)
{
  int rc = PMPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, request);
  Request2Callpath.allocate(*request, chop);
  return rc;
}

#endif

/************************************************
 * Completion calls
 ************************************************/

int MPI_Request_free(MPI_Request* req)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Request_free(req);

  if (req_copy != MPI_REQUEST_NULL) {
    Request2Callpath.free(req_copy, chop);
  }

  return rc;
}

int MPI_Wait(MPI_Request* req, MPI_Status* stat)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Wait(req, stat);

  if (req_copy != MPI_REQUEST_NULL) {
    Request2Callpath.free(req_copy, chop);
  }

  return rc;
}

int MPI_Test(MPI_Request* req, int* flag, MPI_Status* stat)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Test(req, flag, stat);

  if (*flag && req_copy != MPI_REQUEST_NULL) {
    Request2Callpath.free(req_copy, chop);
  }

  return rc;
}

int MPI_Waitany(int count, MPI_Request req[], int* index, MPI_Status* stat)
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Waitany(count, req, index, stat);

  int idx = *index;
  if (idx != MPI_UNDEFINED) {
    Request2Callpath.free(req_copies[idx], chop);
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
    Request2Callpath.free(req_copies[idx], chop);
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

  mpileaks_request_free_array(count, req_copies, req, chop);
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
  mpileaks_request_free_array(count, req_copies, req, chop);
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
    mpileaks_request_free_array(count, req_copies, req, chop);
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
    mpileaks_request_free_array(count, req_copies, req, chop);
  }
  if (req_copies != NULL) {
    free(req_copies);
  }

  return rc;
}
