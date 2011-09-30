#include <iostream>
#include <map>

#include "mpi.h"

#include "CallpathRuntime.h"
#include "Translator.h"
#include "FrameInfo.h"

#include "mpileaks.h"

using namespace std; 

/* File I/O */ 
static map<MPI_File, Callpath> file2callpath; 
/* Defined in mpileaks.cpp */ 
extern map<Callpath, int> callpath2count;
extern int enabled; 
extern CallpathRuntime runtime;

static void fileio_handle_allocate(MPI_File fh)
{
  if (enabled) {
    //printf("%d: allocate: %x\n", rank, (unsigned long long) req);
    if (fh != MPI_FILE_NULL) {
      /* get the call path where this request was allocated */
      Callpath path = runtime.doStackwalk();
      callpath_increase_count(path); 
      
      /* now record handle-to-path record */
      file2callpath[fh] = path; 
    }
  }
}

static void fileio_handle_free(MPI_File fh)
{ 
  if (enabled) {
    //printf("%d: free: %x\n", rank, (unsigned long long) req);
    if (fh == MPI_FILE_NULL) {
      /* TODO: user error */
      return;
    }

    /* lookup stack based on handle value */
    map<MPI_File, Callpath>::iterator it_file2callpath = file2callpath.find(fh);
    if (it_file2callpath != file2callpath.end()) {
      /* found handle */ 
      Callpath path = it_file2callpath->second;
      callpath_decrease_count(path); 

      /* erase entry from handle to stack map */
      file2callpath.erase(it_file2callpath);
    } else {
      cout << "mpileaks: ERROR: Non-null handle being freed but not found in handle2stack map" << endl;
    }
  }
}


int MPI_File_open(MPI_Comm comm, char *filename, int amode, MPI_Info info, MPI_File *fh)
{
  int rc = PMPI_File_open(comm, filename, amode, info, fh); 
  fileio_handle_allocate(*fh); 
  return rc; 
}

int MPI_File_close(MPI_File *fh) 
{
  MPI_File handle_copy = *fh; 

  int rc = PMPI_File_close(fh);
  
  if (handle_copy != MPI_FILE_NULL) {
    fileio_handle_free(handle_copy); 
  }
  
  return rc; 
}
