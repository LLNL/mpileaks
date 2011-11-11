#include <iostream>
#include <map> 
#include <list>

#include "mpi.h"
#include "CallpathRuntime.h"
#include "Translator.h"
#include "FrameInfo.h"



using namespace std;

/* global variables */ 
int enabled = 0; 
CallpathRuntime *runtime = NULL;

static map<Callpath, int> callpath2count;
static Translator trans; 
static int myrank, np; 

typedef struct {
  Callpath path;
  int count;
} callpath_count;



/******************************************
 *** Call path related functions
 ******************************************/ 

typedef map<Callpath,int>::iterator callpath2count_it; 

int callpath_get_count(Callpath path)
{
  callpath2count_it it = callpath2count.find(path); 
  if (it != callpath2count.end()) 
    return it->second; 
  return -1; 
}

void callpath_get_allcounts()
{
  callpath2count_it it; 
  for (it = callpath2count.begin(); it != callpath2count.end(); it++) {
    cout << "callpath_count=" << it->second << endl; 
  }
}

void callpath_increase_count(Callpath path)
{
  /* search for this path in our stacktrace-to-count map */
  map<Callpath,int>::iterator it_callpath2count = callpath2count.find(path);
  if (it_callpath2count != callpath2count.end()) {
    /* found it, increment the count for this path */
    it_callpath2count->second++;
  } else {
    /* not found, so insert the path with a count of 1 */
    callpath2count[path] = 1;
  } 
}

void callpath_decrease_count(Callpath path)
{
  /* now lookup path in callpath2count */
  map<Callpath,int>::iterator it_callpath2count = callpath2count.find(path);
  if (it_callpath2count != callpath2count.end()) {
    if (it_callpath2count->second > 1) {
      /* decrement the count for this path */
      it_callpath2count->second--;
    } else {
      /* remove the entry from the path-to-count map */
      callpath2count.erase(it_callpath2count);
    }
  } else {
    cerr << "mpileaks: ERROR: found a path in handle2cpc map, " <<
      "but no count found in callpath2count map" << endl;
  }
}


/***********************************************************
 *** Functions to gather and print outstanding stack traces 
 ***********************************************************/

static void mpileaks_print_path(Callpath path, int count)
{
  int i, size = path.size();

  cout << "Count: " << count << endl; 
  for (i = 0; i < size; i++) {
    FrameId frame = path[i];
    FrameInfo info = trans.translate(frame);
    cout << "  " << info << endl;
  }
  cout << endl;
}


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
  /* ea: in case 'buffer' needs to be a (void *), create 'buf' to 
     free the memory (freeing a void* is undefined */ 
  char* buf = NULL; 
  void* buffer = NULL;
  if (pack_size > 0) {
    buf = new char[pack_size];
    buffer = buf; 
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

  /* ea: */ 
  /* free buffer */
  if (buf != NULL) {
    delete buf;
  }
}


/* receive and unpack a list of callpath_count items from specified source process */
static void list_recv(list<callpath_count>& path_list, int src, MPI_Comm comm)
{
  MPI_Status status;

  /* receive number of bytes */
  int pack_size = 0;
  PMPI_Recv(&pack_size, 1, MPI_INT, src, 0, comm, &status);

  /* ea: */ 
  /* allocate memory */
  void* buffer = NULL;
  char* buf = NULL; 
  if (pack_size > 0) {
    buf = new char[pack_size];
    buffer = buf; 
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
  
  /* ea: */ 
  /* free buffer */
  if (buf != NULL) {
    delete buf;
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
  for (it = callpath2count.begin(); it != callpath2count.end(); it++) {
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
  while (receiving && mask < np) {
    if ((mask & myrank) == 0) {
      /* we will receive in this step, assume the source rank exists */
      src = myrank | mask;
      if (src < np) {
        list<callpath_count> recv_path_list;
        list_recv(recv_path_list, src, MPI_COMM_WORLD);
        list_merge(path_list, recv_path_list);
      }
    } else {
      /* done receiving, now it's our turn to send */
      dest = myrank & (~mask);
      receiving = 0;
    }
    mask <<= 1;
  }

  /* send list to our parent, unless we're rank 0, in which case, print the list */
  if (myrank != 0) {
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


/***********************************************************
 *** MPI re-definitions
 ***********************************************************/

int MPI_Init(int* argc, char** argv[])
{
  int rc = PMPI_Init(argc, argv);
  PMPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  PMPI_Comm_size(MPI_COMM_WORLD, &np);

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
