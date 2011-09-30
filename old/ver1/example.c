#include <stdio.h>
#include <unistd.h>

#include "mpi.h"

int main(int argc, char **argv) {
  MPI_Init(NULL, NULL);

  int rank, ranks;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranks);

  MPI_Request request[1];
  // MPI_Status status[1];

  int val = rank;
  if (rank == 0) {
    int i;
    for (i=0; i < 2; i++) {
      MPI_Isend(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, request);
    }
    //MPI_Wait(request, status);
  } else {
    MPI_Irecv(&val, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, request);
    // MPI_Wait(request, status);
    sleep(2); 
  }

//  MPI_Waitall(1, request, status);
//  MPI_Wait(request, status);

  MPI_Finalize();
  return 0;
}
