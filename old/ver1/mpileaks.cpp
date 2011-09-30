#include <string>
#include <cstdio>
#include <iostream>
#include <stdint.h>
#include <cassert>
#include <stack>
#include <map>

#include "mpi.h"

#include "CallpathRuntime.h"
#include "Translator.h"
#include "FrameInfo.h"

using namespace std;

map<Callpath, int> callpath2count;
static map<MPI_Request, stack<Callpath>*> request2callpath;  

CallpathRuntime runtime;
static Translator t;
static int rank, np;

int enabled = 0;

/*
 * Maintain two AVL trees (or C++ STL maps)
 *     Stack Trace AVL Tree
 *       associates a stack trace (function calls only) with a count field
 *     Object AVL Tree
 *       associates an MPI handle id with a node in the Stack Trace AVL Tree
 * 
 * At allocation:
 *     Acquire current stack trace
 *     Lookup stack trace in Stack Trace AVL Tree
 *        If not found,
 *          create a new node for this stack trace,
 *          set count to 1,
 *          and insert into tree
 *        If found,
 *          increment count field,
 *          and record address of node
 *     Create a new node for the Object AVL Tree
 *     Record object handle id and address of stack trace node
 *       in the object node
 *     Insert object node into Object AVL Tree
 * 
 * At deallocation:
 *     Lookup node in Object AVL Tree using object handle id,
 *       remember address of stack trace node
 *     Delete object node from Object AVL Tree
 *     Decrement count field on stack trace node
 *        If count == 0, delete node from Stack Trace AVL Tree
 * 
 * At dump time (finalize or PControl call)
 *     Iterate over all nodes in Stack Trace AVL Tree
 *       print count with stack trace
 */

void callpath_increase_count(Callpath path)
{
  /* search for this path in our stacktrace-to-count map */
  map<Callpath,int>::iterator it_callpath2count = callpath2count.find(path);
  if (it_callpath2count != callpath2count.end()) {
    /* found it, increment the count for this path */
    (*it_callpath2count).second++;
  } else {
    /* not found, so insert the path with a count of 1 */
    callpath2count[path] = 1;
  } 
}

static void mpileaks_request_allocate(MPI_Request req)
{
  if (enabled) {
    //printf("%d: allocate: %x\n", rank, (unsigned long long) req);
    if (req != MPI_REQUEST_NULL) {
      /* get the call path where this request was allocated */
      Callpath path = runtime.doStackwalk();
      callpath_increase_count(path); 
      
      /* now record handle-to-path record */
      map<MPI_Request, stack<Callpath>*>::iterator it_request2callpath = 
	request2callpath.find(req); 
      if (it_request2callpath != request2callpath.end()) {
	/* found request */ 
	it_request2callpath->second->push(path); 
      } else {
	/* not found, create new stack of callpaths, push current */ 
	request2callpath[req] = new stack<Callpath>; 
	request2callpath[req]->push(path); 
      }
    }
  }
}


void callpath_decrease_count(Callpath path)
{
  /* now lookup path in callpath2count */
  map<Callpath,int>::iterator it_callpath2count = callpath2count.find(path);
  if (it_callpath2count != callpath2count.end()) {
    if ((*it_callpath2count).second > 1) {
      /* decrement the count for this path */
      (*it_callpath2count).second--;
    } else {
      /* remove the entry from the path-to-count map */
      callpath2count.erase(it_callpath2count);
    }
  } else {
    cerr << "mpileaks: ERROR: Found a path in handle2stack map, but no count found in callpath2count map" << endl;
  }
}


static void mpileaks_request_free(MPI_Request req)
{
  if (enabled) {
    //printf("%d: free: %x\n", rank, (unsigned long long) req);
    if (req == MPI_REQUEST_NULL) {
      /* TODO: user error */
      return;
    }

    Callpath path; 
    /* lookup stack based on handle value */
    map<MPI_Request, stack<Callpath>*>::iterator it_request2callpath = 
      request2callpath.find(req);
    if (it_request2callpath != request2callpath.end()) {
      /* found request */ 
      if (it_request2callpath->second->size() > 0) {
	path = it_request2callpath->second->top(); 
      } else {
	cerr << "mpileaks: Error: callpath stack should not be empty" << endl; 
      }
      
      callpath_decrease_count(path); 
      
      /* erase entry from handle to stack map */
      it_request2callpath->second->pop(); 
      if (it_request2callpath->second->size() == 0) {
	delete it_request2callpath->second; 
	request2callpath.erase(it_request2callpath); 
      }
      
    } else {
      cout << "mpileaks: ERROR: Non-null handle being freed but not found in handle2stack map" << endl;
    }
  }
}

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

static void mpileaks_request_allocate_array(int count, MPI_Request *reqs)
{
  int i; 
  for (i = 0; i < count; i++) {
    mpileaks_request_allocate(reqs[i]); 
  }
}


/* given a copy of the request array and a new version, free any requests that
 * have changed to MPI_REQUEST_NULL */
static void mpileaks_request_free_array(int count, MPI_Request req1[], MPI_Request req2[])
{
  int i;
  for (i = 0; i < count; i++) {
    if (req1[i] != MPI_REQUEST_NULL && req2[i] == MPI_REQUEST_NULL) {
      mpileaks_request_free(req1[i]);
    }
  }
}

static void mpileaks_print_path(Callpath path, int count)
{
  cout << "Count: " << count << endl;

  int size = path.size();
  int i;
  for (i = 0; i < size; i++) {
    FrameId frame = path[i];
    FrameInfo info = t.translate(frame);
    cout << "  " << info << endl;
  }

  cout << endl;
  return;
}

/* cycle through and print each stack trace for which there is an outstanding request */
static void mpileaks_dump_outstanding()
{
  map<Callpath,int>::iterator it;
  for (it = callpath2count.begin(); it != callpath2count.end(); it++) {
    Callpath path = (*it).first;
    int count = (*it).second;
    mpileaks_print_path(path, count);
  }
}

/* send calls */

int MPI_Isend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Isend(buf, count, dt, dest, tag, comm, req);
  mpileaks_request_allocate(*req);
  return rc;
}

int MPI_Issend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Issend(buf, count, dt, dest, tag, comm, req);
  mpileaks_request_allocate(*req);
  return rc;
}

int MPI_Irsend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Irsend(buf, count, dt, dest, tag, comm, req);
  mpileaks_request_allocate(*req);
  return rc;
}

int MPI_Ibsend(void* buf, int count, MPI_Datatype dt, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Ibsend(buf, count, dt, dest, tag, comm, req);
  mpileaks_request_allocate(*req);
  return rc;
}

/* receive calls */

int MPI_Irecv(void* buf, int count, MPI_Datatype dt, int src, int tag, MPI_Comm comm, MPI_Request* req)
{
  int rc = PMPI_Irecv(buf, count, dt, src, tag, comm, req);
  mpileaks_request_allocate(*req);
  return rc;
}

/* 
 * Persistent communication
 * An 'init' call should match an MPI_Request_free. 
 * A 'start' call should match a (multiple) complete operation (e.g., MPI_Wait). 
 */ 

int MPI_Send_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		  MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request); 
  mpileaks_request_allocate(*request); 
  return rc; 
}

int MPI_Bsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		   MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request); 
  mpileaks_request_allocate(*request); 
  return rc; 
}

int MPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		   MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request); 
  mpileaks_request_allocate(*request); 
  return rc; 
}

int MPI_Rsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, 
		   MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request); 
  mpileaks_request_allocate(*request); 
  return rc; 
}

int MPI_Recv_init(void* buf, int count, MPI_Datatype datatype, int source, int tag, 
		  MPI_Comm comm, MPI_Request *request)
{
  int rc = PMPI_Recv_init(buf, count, datatype, source, tag, comm, request); 
  mpileaks_request_allocate(*request); 
  return rc; 
}

int MPI_Start(MPI_Request *request)
{
  int rc = PMPI_Start(request); 
  mpileaks_request_allocate(*request); 
  return rc; 
}

int MPI_Startall(int count, MPI_Request *array_of_requests)
{
  int rc = PMPI_Startall(count, array_of_requests); 
  mpileaks_request_allocate_array(count, array_of_requests); 
  return rc; 
}



/* free calls */

int MPI_Request_free(MPI_Request* req)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Request_free(req);

  if (req_copy != MPI_REQUEST_NULL) {
    mpileaks_request_free(req_copy);
  }

  return rc;
}

int MPI_Wait(MPI_Request* req, MPI_Status* stat)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Wait(req, stat);

  if (req_copy != MPI_REQUEST_NULL) {
    mpileaks_request_free(req_copy);
  }

  return rc;
}

int MPI_Test(MPI_Request* req, int* flag, MPI_Status* stat)
{
  MPI_Request req_copy = *req;

  int rc = PMPI_Test(req, flag, stat);

  if (*flag && req_copy != MPI_REQUEST_NULL) {
    mpileaks_request_free(req_copy);
  }

  return rc;
}

int MPI_Waitany(int count, MPI_Request req[], int* index, MPI_Status* stat)
{
  MPI_Request* req_copies = mpileaks_request_copy_array(count, req);

  int rc = PMPI_Waitany(count, req, index, stat);

  int idx = *index;
  if (idx != MPI_UNDEFINED) {
    mpileaks_request_free(req_copies[idx]);
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
    mpileaks_request_free(req_copies[idx]);
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

/* dump calls */

int MPI_Init(int* argc, char** argv[])
{
  int rc = PMPI_Init(argc, argv);
  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  PMPI_Comm_size(MPI_COMM_WORLD, &np);
  enabled = 1;
  return rc;
}

int MPI_PControl(const int level, ...)
{
  if (level == 0) {
    enabled = 0;
  } else if (level == 1) {
    enabled = 1;
  } else if (level == 2) {
    mpileaks_dump_outstanding();
  }

  /* TODO: need to call PMPI_PControl here? */

  return MPI_SUCCESS;
}

int MPI_Finalize()
{
  mpileaks_dump_outstanding();
  enabled = 0;
  int rc = PMPI_Finalize();
  return rc;
}
