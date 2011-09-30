#include <string>
#include <cstdio>
#include <iostream>
#include <stdint.h>
#include <cassert>
#include <vector>
#include <map>
#include <list>

#include "mpi.h"

#include "CallpathRuntime.h"
#include "Translator.h"
#include "FrameInfo.h"
#include "mpi_utils.h"

using namespace std;

static std::map<Callpath, int> stack2count;
static std::map<MPI_Request, Callpath> handle2stack;

static Translator t;
static CallpathRuntime* runtime = NULL;
static int rank, ranks;

static int enabled = 0;

/*
 * Maintain two C++ STL maps
 *     Stack Trace to Count Map
 *       associates a stack trace (function calls only) with a count field
 *     Object Handle to Stack Trace Map
 *       associates an MPI handle id with an element in Stack Trace map
 * 
 * At allocation:
 *     Acquire current stack trace
 *     Lookup stack trace in Stack Trace map
 *        If not found,
 *          create a new node for this stack trace,
 *          set count to 1,
 *          and insert into map
 *        If found,
 *          increment count field,
 *          and record address of node
 *     Create a new node for the Object map
 *     Record object handle id and address of stack trace node
 *       in the object node
 *     Insert object node into Object map
 * 
 * At deallocation:
 *     Lookup node in Object map using object handle id,
 *       remember address of stack trace node
 *     Delete object node from Object map
 *     Decrement count field on stack trace node
 *        If count == 0, delete node from Stack Trace map
 * 
 * At dump time (finalize or PControl call)
 *     Iterate over all nodes in Stack Trace map
 *       print count with stack trace
 */

static void mpileaks_request_allocate(MPI_Request req)
{
  if (enabled) {
    if (req != MPI_REQUEST_NULL) {
      /* get the call path where this request was allocated */
      Callpath path = (*runtime).doStackwalk();

      /* search for this path in our stacktrace-to-count map */
      map<Callpath,int>::iterator it_stack2count = stack2count.find(path);
      if (it_stack2count != stack2count.end()) {
        /* found it, increment the count for this path */
        (*it_stack2count).second++;
      } else {
        /* not found, so insert the path with a count of 1 */
        stack2count[path] = 1;
      }

      /* now record handle-to-path record */
      handle2stack[req] = path;
    }
  }
}

static void mpileaks_request_free(MPI_Request req)
{
  if (enabled) {
    if (req == MPI_REQUEST_NULL) {
      /* TODO: user error */
      return;
    }

    /* lookup stack based on handle value */
    map<MPI_Request,Callpath>::iterator it_handle2stack = handle2stack.find(req);
    if (it_handle2stack != handle2stack.end()) {
      Callpath path = (*it_handle2stack).second;

      /* now lookup path in stack2count */
      map<Callpath,int>::iterator it_stack2count = stack2count.find(path);
      if (it_stack2count != stack2count.end()) {
        if ((*it_stack2count).second > 1) {
          /* decrement the count for this path */
          (*it_stack2count).second--;
        } else {
          /* remove the entry from the path-to-count map */
          stack2count.erase(it_stack2count);
        }
      } else {
        cout << "mpileaks: ERROR: Found a path in handle2stack map, but no count found in stack2count map" << endl;
      }

      /* erase entry from handle to stack map */
      handle2stack.erase(it_handle2stack);
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

typedef struct {
  Callpath path;
  int count;
} callpath_count;

/* sort callpath_count items by path */
static bool compare_callpaths(callpath_count first, callpath_count second)
{
  callpath_path_lt lt;
  Callpath first_path  = first.path;
  Callpath second_path = second.path;
  return lt(first_path, second_path);
}

/* sort callpath_count items by count (descending), then path (ascending) */
static bool compare_counts(callpath_count first, callpath_count second)
{
  /* sort by counts in reverse order */
  int first_count  = first.count;
  int second_count = second.count;
  if (first_count > second_count) {
    return true;
  } else if (first_count < second_count) {
    return false;
  }

  /* sort by callpath in ascending order */
  callpath_path_lt lt;
  Callpath first_path  = first.path;
  Callpath second_path = second.path;
  return lt(first_path, second_path);
}

/* pack and send a list of callpath_count items to specified destination process */
static void list_send(list<callpath_count>& path_list, int dest, MPI_Comm comm)
{
  list<callpath_count>::iterator it_list;

  /* get size of buffer to pack list into */
  int pack_size = 0;
  pack_size += pmpi_packed_size(1, MPI_INT, comm);
  pack_size += ModuleId::packed_size_id_map(comm);
  for (it_list = path_list.begin(); it_list != path_list.end(); it_list++) {
    Callpath path = it_list->path;
    pack_size += path.packed_size(comm);
    pack_size += pmpi_packed_size(1, MPI_INT, comm);
  }

  /* allocate memory */
  void* buffer = NULL;
  if (pack_size > 0) {
    buffer = new char[pack_size];
  }

  /* if we have any callpaths, pack modules and list into buffer */
  int position = 0;
  int size = path_list.size();
  PMPI_Pack(&size, 1, MPI_INT, buffer, pack_size, &position, comm);
  if (size > 0) {
    ModuleId::pack_id_map(buffer, pack_size, &position, comm);
    for (it_list = path_list.begin(); it_list != path_list.end(); it_list++) {
      it_list->path.pack(buffer, pack_size, &position, comm);
      int count = it_list->count;
      PMPI_Pack(&count, 1, MPI_INT, buffer, pack_size, &position, comm);
    }
  }

  /* send size */
  PMPI_Send(&position, 1, MPI_INT, dest, 0, comm);

  /* send list */
  PMPI_Send(buffer, position, MPI_PACKED, dest, 0, comm);

  /* free buffer */
  if (buffer != NULL) {
    delete buffer;
  }
}

/* receive and unpack a list of callpath_count items from specified source process */
static void list_recv(list<callpath_count>& path_list, int src, MPI_Comm comm)
{
  MPI_Status status;

  /* receive number of bytes */
  int pack_size = 0;
  PMPI_Recv(&pack_size, 1, MPI_INT, src, 0, comm, &status);

  /* allocate memory */
  void* buffer = NULL;
  if (pack_size > 0) {
    buffer = new char[pack_size];
  }

  /* receive list */
  PMPI_Recv(buffer, pack_size, MPI_PACKED, src, 0, comm, &status);

  /* unpack list from buffer */
  int position = 0;
  int size;
  PMPI_Unpack(buffer, pack_size, &position, &size, 1, MPI_INT, comm);
  if (size > 0) {
    ModuleId::id_map modules;
    ModuleId::unpack_id_map(buffer, pack_size, &position, modules, comm);
    while (size > 0) {
      /* unpack the path */
      Callpath path = Callpath::unpack(modules, buffer, pack_size, &position, comm);

      /* unpack the count */
      int count;
      PMPI_Unpack(buffer, pack_size, &position, &count, 1, MPI_INT, comm);

      /* create a new callpath_count elem and insert into the list */
      callpath_count* elem = new callpath_count;
      elem->path  = path;
      elem->count = count;
      path_list.push_back(*elem);

      /* decrement out count by one */
      size--;
    }
  }

  /* free buffer */
  if (buffer != NULL) {
    delete buffer;
  }
}

/* merge list2 into list1, add new entries to list1 where needed, add count
 * fields where there is a match */
static void list_merge(list<callpath_count>& list1, list<callpath_count>& list2)
{
  /* start with the first element in each list */
  list<callpath_count>::iterator it_list1 = list1.begin();
  list<callpath_count>::iterator it_list2 = list2.begin();

  while (it_list1 != list1.end() && it_list2 != list2.end()) {
    /* get next element from each list, since the lists are sorted in ascending order,
     * this element will be the smallest from each list */
    callpath_count item1 = (*it_list1);
    callpath_count item2 = (*it_list2);
    if (compare_callpaths(item2, item1)) {
      /* list2 has smaller element, insert from list2 and move to next element in list2 */
      list1.insert(it_list1, item2);
      it_list2++;
    } else if (compare_callpaths(item1, item2)) {
      /* list1 has smaller element, just move to next element in list1 */
      it_list1++;
    } else {
      /* both lists have the same element, add the counts and move to the next element in each list */
      (*it_list1).count += (*it_list2).count;
      it_list1++;
      it_list2++;
    }
  }

  /* insert any remaining items in list2 to the end of list1 */
  while (it_list2 != list2.end()) {
    callpath_count item2 = (*it_list2);
    list1.insert(it_list1, item2);
    it_list2++;
  }
}

/* cycle through and print each stack trace for which there is an outstanding request */
static void mpileaks_dump_outstanding()
{
  /* create a list of callpaths and counts */
  list<callpath_count> path_list;
  map<Callpath,int>::iterator it;
  for (it = stack2count.begin(); it != stack2count.end(); it++) {
    Callpath path = (*it).first;
    int count = (*it).second;
    callpath_count* elem = new callpath_count;
    elem->path = path;
    elem->count = count;
    path_list.push_back(*elem);
  }

  /* sort our list by callpath */
  path_list.sort(compare_callpaths);

  /* receive lists from children and merge with our own */
  int mask = 0x1, src, dest;
  int receiving = 1;
  while (receiving && mask < ranks) {
    if ((mask & rank) == 0) {
      /* we will receive in this step, assume the source rank exists */
      src = rank | mask;
      if (src < ranks) {
        list<callpath_count> recv_path_list;
        list_recv(recv_path_list, src, MPI_COMM_WORLD);
        list_merge(path_list, recv_path_list);
      }
    } else {
      /* done receiving, now it's our turn to send */
      dest = rank & (~mask);
      receiving = 0;
    }
    mask <<= 1;
  }

  /* send list to our parent, unless we're rank 0, in which case, print the list */
  if (rank != 0) {
    list_send(path_list, dest, MPI_COMM_WORLD);
  } else {
    /* sort callpaths by total count */
    path_list.sort(compare_counts);

    /* now print each callpath with its count */
    list<callpath_count>::iterator it_list;
    for (it_list = path_list.begin(); it_list != path_list.end(); it_list++) {
      Callpath path = (*it_list).path;
      int count = (*it_list).count;
      mpileaks_print_path(path, count);
    }
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
  PMPI_Comm_size(MPI_COMM_WORLD, &ranks);

  /* we wait to create our runtime object until after MPI_Init,
   * because it registers the SIGSEGV signal within stackwalker
   * and we want to override any previous registrations for this
   * signal by MPI  */
  if (runtime == NULL) {
    runtime = new CallpathRuntime;
  }
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
