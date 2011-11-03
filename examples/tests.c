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


void test_errhandler(MPI_Comm *comm, int *errcode, ...)
{
  *errcode = 1;
  if (*comm == MPI_COMM_WORLD) {
    *errcode = 7;
  }
}


void errhandlers(int myrank, int np)
{
  MPI_Errhandler errhandler1, errhandler2;
  MPI_Errhandler_create(test_errhandler, &errhandler1); 
  MPI_Errhandler_create(test_errhandler, &errhandler2); 
  MPI_Errhandler_free(&errhandler1);
  //MPI_Errhandler_free(&errhandler2);
}


void keyvals(int myrank, int np)
{
  int keyval1, keyval2;
  void* extra_state1 = NULL;
  void* extra_state2 = NULL;
  MPI_Keyval_create(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &keyval1, &extra_state1);
  MPI_Keyval_create(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &keyval2, &extra_state2);
  //MPI_Keyval_free(&keyval1);
  MPI_Keyval_free(&keyval2);
}


// example user-defined reduction function
void test_user_defined_op_int_sum(void* in, void* inout, int* len, MPI_Datatype* datatype)
{
  int i;
  int* a = (int*) in;
  int* b = (int*) inout;
  for (i=0; i < *len; i++) {
    int result = a[i] + b[i];
    b[i] = result;
  }
}


void mems(int myrank, int np)
{
#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

  MPI_Aint size = 1024;
  void *buf1, *buf2;
  MPI_Alloc_mem(size, MPI_INFO_NULL, &buf1);
  MPI_Alloc_mem(size, MPI_INFO_NULL, &buf2);
  MPI_Free_mem(buf1);
  //MPI_Free_mem(buf2);
#endif
}


void ops(int myrank, int np)
{
  MPI_Op op1, op2;
  MPI_Op_create(test_user_defined_op_int_sum, 0, &op1);
  MPI_Op_create(test_user_defined_op_int_sum, 0, &op2);
  MPI_Op_free(&op1);
  //MPI_Op_free(&op2);
}


/* 
   Test whether the following are true: 
   - free groups with handle = MPI_GROUP_ENTRY 
   - duplicate groups return the same MPI_Group value
   This seems to be true for MVAPICH 1 and 2.
   Example output: 
   --
   grp1 and grp2 are the same
   grp3 and grp4 differ
   grp5 is MPI_GROUP_EMPTY
   grp6 and grp7 are the same
   1: comm3 is NULL
   grp1 and grp2 are the same
   grp3 and grp4 differ
   grp5 is MPI_GROUP_EMPTY
   --
*/ 
void group_properties(int myrank, int np)
{
  MPI_Group grp1, grp2, grp3, grp4, grp5, grp6, grp7;
  MPI_Comm comm3; 
  int process_ranks[1] = {0}; 

  MPI_Comm_group(MPI_COMM_WORLD, &grp1); 
  MPI_Comm_group(MPI_COMM_WORLD, &grp2); 
  
  MPI_Group_incl(grp1, 1, process_ranks, &grp3); 
  MPI_Group_incl(grp1, 1, process_ranks, &grp4); 
  MPI_Group_incl(grp1, 0, process_ranks, &grp5); 

  MPI_Comm_create(MPI_COMM_WORLD, grp3, &comm3); 
  if (comm3 == MPI_COMM_NULL)
    printf("%d: comm3 is NULL\n", myrank); 
  else {
    MPI_Comm_group(comm3, &grp6); 
    MPI_Comm_group(comm3, &grp7); 
  }

  if (grp1 == grp2) 
    printf("grp1 and grp2 are the same\n"); 
  else
    printf("grp1 and grp2 differ\n"); 

  if (grp3 == grp4) 
    printf("grp3 and grp4 are the same\n"); 
  else
    printf("grp3 and grp4 differ\n"); 

  if (grp5 == MPI_GROUP_EMPTY)
    printf("grp5 is MPI_GROUP_EMPTY\n"); 
  else
    printf("grp5 is not MPI_GROUP_EMPTY\n"); 

  if (comm3 != MPI_COMM_NULL) {
    if (grp6 == grp7) 
      printf("grp6 and grp7 are the same\n"); 
    else
      printf("grp6 and grp7 differ\n"); 
  }

  MPI_Group_free(&grp1); 
  MPI_Group_free(&grp2); 
  MPI_Group_free(&grp3); 
  MPI_Group_free(&grp4); 
  MPI_Group_free(&grp5); 
  if (comm3 != MPI_COMM_NULL) {
    MPI_Group_free(&grp6); 
    MPI_Group_free(&grp7); 
  }
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


void infos(int myrank, int np)
{
#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

  MPI_Info info1, info2, info3;
  MPI_Info_create(&info1);
  MPI_Info_create(&info2);
  MPI_Info_dup(info1, &info3);
  MPI_Info_free(&info1);
  //MPI_Info_free(&info2);
  //MPI_Info_free(&info3);
#endif
}

void comms(int myrank, int np)
{
  MPI_Comm comm1, comm2, comm3; 
  MPI_Comm_dup(MPI_COMM_WORLD, &comm1);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm2);
  MPI_Comm_split(MPI_COMM_WORLD, myrank, myrank, &comm3);

  //MPI_Comm_free(&comm1); 
  // MPI_Comm_free(&comm2); 
  // MPI_Comm_free(&comm3); 
}


void wins(int myrank, int np)
{
#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

  int winbuf1[1024];
  int winbuf2[1024];
  MPI_Aint size = 1024 * sizeof(int);
  int disp_unit = sizeof(int);
  MPI_Win win1, win2;
  MPI_Win_create(winbuf1, size, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win1);
  MPI_Win_create(winbuf2, size, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win2);
  MPI_Win_free(&win1);
  //MPI_Win_free(&win2);
#endif
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

#if 1
  group_properties(myrank, np); 
#endif 

  comms(myrank, np);
  datatypes(myrank, np); 
  errhandlers(myrank, np);
  fileio(myrank, np); 
  groups(myrank, np);
  infos(myrank, np);
  keyvals(myrank, np);
  persistent(myrank, np); 
  sendrecv(myrank, np); 
  ops(myrank, np);
  mems(myrank, np);
  wins(myrank, np);

  MPI_Finalize(); 

  return 0; 
}
