#include <stdio.h>
#include <unistd.h> 

#include "mpi.h"


/* Persistent communication. Goal is to identify cases where an error is not needed and 
   a warning is issued instead.
   Todo: add a separate container for warnings; mpi_request_free is a special case
   and should pop all elements in the stack (for send only). 
*/ 
void case1(int rank, int np)
{
  int i, N=1, val=rank;
  MPI_Request request[1];
  MPI_Status status[1];
  
  if (rank == 0) {
    
    MPI_Send_init(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request[0]); 
    for (i = 0; i < N; i++) {
      MPI_Start(&request[0]);  
      sleep(5);
      //MPI_Wait(&request[0], &status[0]);    
    }
    MPI_Request_free(&request[0]);  
    
  } else {
    
    for (i = 0; i < N; i++) {
      MPI_Irecv(&val, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request[0]);
      MPI_Wait(&request[0], &status[0]);
    }
    
  }
  
  //  MPI_Waitall(1, request, status);
}


/* File I/O */ 
void case2(int rank, int np)
{
  int size_int, amode;
  MPI_Datatype etype, filetype;
  MPI_Info info;
  MPI_Status status;
  MPI_File fh;
  MPI_Offset offset;
  
  amode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
  size_int = sizeof(size_int);
  info = MPI_INFO_NULL;
  
  MPI_File_open(MPI_COMM_WORLD, "data.dat", amode, info, &fh);

  offset = rank * size_int;
  etype = MPI_INT;
  filetype = MPI_INT;
  
  /* Try running this sample with and without this line .*/
  /* Compare data.dat each time : od -c data.dat .*/ 
  MPI_File_set_view(fh, offset, etype, filetype, "native", info);
  
  MPI_File_write_at(fh, offset, &rank, 1, MPI_INT, &status);
  
  printf("Hello from rank %d. I wrote: %d\n", rank, rank);
  
  // MPI_File_close(&fh);
}  



int main(int argc, char **argv)
{
  int rank, np; 
  
  MPI_Init(NULL, NULL); 
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  
  case1(rank, np); 
  case2(rank, np); 

  MPI_Finalize();

  return 0;
}
