#include <stdio.h>
#include <unistd.h>
#include "mpi.h"

/*******************************************************
 * The functions below are intentionally leaky to  
 * check execution of mpileaks for these cases. 
 *******************************************************/

#define COUNT 10
void persistent(int myrank, int np)
{
  int i, buf[COUNT]; 
  MPI_Request req[2]; 
  MPI_Status status[2]; 

  MPI_Send_init(buf, COUNT, MPI_INT, 1, 0, MPI_COMM_WORLD, &req[0]); 
  MPI_Recv_init(buf, COUNT, MPI_INT, 0, 0, MPI_COMM_WORLD, &req[1]);
 
  if (myrank == 0) {
    for (i=0; i<COUNT; i++) 
      buf[i] = i; 
    MPI_Start(&req[0]); 
    // MPI_Wait(&req[0], &status[0]); 
  } else {
    MPI_Start(&req[1]); 
    MPI_Wait(&req[1], &status[1]); 
  }

  sleep(3); 
  MPI_Request_free(&req[0]); 
  // MPI_Request_free(&req[1]); 
}



#define NUM_MSGS 3
void sendrecv(int myrank, int np)
{
  int i;
  MPI_Request request[NUM_MSGS];
  MPI_Status status[NUM_MSGS];
  int val[NUM_MSGS]; 

  val[0] = myrank;
  if (myrank == 0) {
    for (i=0; i < NUM_MSGS; i++) {
      MPI_Isend(&val, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request[i]);
    }
    // MPI_Wait(&request[NUM_MSGS-1], status);
  } else {
    for (i=0; i< NUM_MSGS; i++) {
      MPI_Irecv(&val[i], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request[i]);
      // MPI_Wait(request, status);
    }
  }
  sleep(3); 

  MPI_Waitall(1, request, status);
  //  MPI_Wait(request, status);
}


#define BUFSIZE 1000
void fileio(int myrank, int np)
{
  int i, buf[BUFSIZE];  
  char filename[128]; 
  MPI_File myfile; 

  for (i=0; i<BUFSIZE; i++)
    buf[i] = myrank * BUFSIZE + i;
  sprintf(filename, "testfile.%d", myrank); 

  MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_WRONLY | MPI_MODE_CREATE, 
		MPI_INFO_NULL, &myfile); 

  MPI_File_write(myfile, buf, BUFSIZE, MPI_INT, MPI_STATUS_IGNORE); 

  //MPI_File_close(&myfile); 
}


void datatypes(int myrank, int np)
{
  MPI_Datatype type1, type2; 
  MPI_Type_contiguous(5, MPI_INT, &type1); 
  MPI_Type_commit(&type1); 

  MPI_Type_vector(3, 5, 4, type1, &type2); 
  MPI_Type_commit(&type2); 

  MPI_Type_free(&type1); 
  // MPI_Type_free(&type2); 
}

void groups(int myrank, int np)
{
  MPI_Group group1, group2, group3; 
  MPI_Comm_group(MPI_COMM_WORLD, &group1);

  int ranks[2] = {0,1};
  MPI_Group_incl(group1, 1, &ranks[0], &group2);
  MPI_Group_excl(group1, 1, &ranks[0], &group3);

  MPI_Group_free(&group1); 
  // MPI_Group_free(&group2); 
  // MPI_Group_free(&group3); 
}

/*******************************************************
 * Main
 *******************************************************/


int main(int argc, char *argv[])
{
  int myrank, np; 

  MPI_Init(&argc, &argv); 
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 
  MPI_Comm_size(MPI_COMM_WORLD, &np); 

  groups(myrank, np);
  datatypes(myrank, np); 
  persistent(myrank, np); 
  fileio(myrank, np); 
  sendrecv(myrank, np); 
  
  MPI_Finalize(); 

  return 0; 
}
