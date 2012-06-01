/* Copyright (c) 2012, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Adam Moody <moody20@llnl.gov> and Edgar A. Leon <leon@llnl.gov>.
 * LLNL-CODE-557543.
 * All rights reserved.
 * This file is part of the mpileaks tool package.
 * For details, see https://github.com/hpc/mpileaks
 * Please also read this file: LICENSE.TXT. */

#include "mpi.h"
#include "mpileaks.h"


/*
 * This class provides 'allocate' and 'free' functions needed to
 * keep track of MPI communicator objects.
 *
 * 'is_handle_null' needs to be defined before the class can be 
 * instantiated. This is necessary since it depends on the type of 
 * handle used (e.g., MPI_File). 
 */ 
static class MPI_Comm2CallpathSet : public Handle2Set<MPI_Comm>
{
public: 
  bool is_handle_null(MPI_Comm handle) {
    return (handle == MPI_COMM_NULL) ? 1 : 0; 
  }
} Comm2Callpath; 



/************************************************
 * MPI communicator allocation calls 
 ************************************************/

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
  int rc = PMPI_Comm_dup(comm, newcomm); 
  Comm2Callpath.allocate(*newcomm); 
  return rc; 
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
  int rc = PMPI_Comm_create(comm, group, newcomm); 
  Comm2Callpath.allocate(*newcomm); 
  return rc; 
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  int rc = PMPI_Comm_split(comm, color, key, newcomm); 
  Comm2Callpath.allocate(*newcomm); 
  return rc; 
}

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
{
  int rc = PMPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm); 
  Comm2Callpath.allocate(*newintercomm); 
  return rc; 
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
  int rc = PMPI_Intercomm_merge(intercomm, high, newintracomm);
  Comm2Callpath.allocate(*newintracomm);
  return rc;
}

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods, int reorder, MPI_Comm *comm_cart)
{
  int rc = PMPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
  Comm2Callpath.allocate(*comm_cart);
  return rc;
}

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index, int *edges, int reorder, MPI_Comm *comm_graph)
{
  int rc = PMPI_Graph_create(comm_old, nnodes, index, edges, reorder, comm_graph);
  Comm2Callpath.allocate(*comm_graph);
  return rc;
}

int MPI_Cart_sub(MPI_Comm comm_old, int *remain_dims, MPI_Comm *newcomm)
{
  int rc = PMPI_Cart_sub(comm_old, remain_dims, newcomm);
  Comm2Callpath.allocate(*newcomm);
  return rc;
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Dist_graph_create_adjacent(
  MPI_Comm comm_old,
  int indegree, int sources[], int sourceweights[],
  int outdegree, int destinations[], int destweights[],
  MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
{
  int rc = PMPI_Dist_graph_create_adjacent(
    comm_old,
    indegree, sources, sourceweights,
    outdegree, destinations, destweights,
    info, reorder, comm_dist_graph
  );
  Comm2Callpath.allocate(*comm_dist_graph);
  return rc;
}

int MPI_Dist_graph_create(
  MPI_Comm comm_old,
  int n, int sources[], int degrees[], int destinations[], int weights[],
  MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
{
  int rc = PMPI_Dist_graph_create(
    comm_old,
    n, sources, degrees, destinations, weights,
    info, reorder, comm_dist_graph
  );
  Comm2Callpath.allocate(*comm_dist_graph);
  return rc;
}

int MPI_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
  int rc = PMPI_Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
  Comm2Callpath.allocate(*intercomm);
  return rc;
}

int MPI_Comm_get_parent(MPI_Comm *parent)
{
  int rc = PMPI_Comm_get_parent(parent);
  Comm2Callpath.allocate(*parent);
  return rc;
}  

int MPI_Comm_spawn_multiple(
  int count, char *array_of_commands[], char **array_of_argv[], int array_of_maxprocs[],
  MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
  int rc = PMPI_Comm_spawn_multiple(
    count, array_of_commands, array_of_argv, array_of_maxprocs,
    array_of_info, root, comm, intercomm, array_of_errcodes
  );
  Comm2Callpath.allocate(*intercomm);
  return rc;
}

int MPI_Comm_accept(char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
  int rc = PMPI_Comm_accept(port_name, info, root, comm, newcomm);
  Comm2Callpath.allocate(*newcomm);
  return rc;
}

int MPI_Comm_connect(char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
  int rc = PMPI_Comm_connect(port_name, info, root, comm, newcomm);
  Comm2Callpath.allocate(*newcomm);
  return rc;
}

int MPI_Comm_join(int fd, MPI_Comm *intercomm)
{
  int rc = PMPI_Comm_join(fd, intercomm);
  Comm2Callpath.allocate(*intercomm);
  return rc;
}

#endif  

/************************************************
 * MPI group free call
 ************************************************/

int MPI_Comm_free(MPI_Comm *comm)
{
  MPI_Comm handle_copy = *comm; 
  int rc = PMPI_Comm_free(comm); 
  Comm2Callpath.free(handle_copy); 
  return rc; 
  
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Comm_disconnect(MPI_Comm *comm)
{
  MPI_Comm handle_copy = *comm;
  int rc = PMPI_Comm_disconnect(comm);
  Comm2Callpath.free(handle_copy);
  return rc;
}

#endif  

