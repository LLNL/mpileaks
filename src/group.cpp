#include "mpi.h"
#include "mpileaks.h"


/*
 * This class provides 'allocate' and 'free' functions needed to
 * keep track of MPI group objects.
 *
 * 'is_handle_null' needs to be defined before the class can be 
 * instantiated. This is necessary since it depends on the type of 
 * handle used (e.g., MPI_File). 
 */ 
static class MPI_Group2CallpathSet : public Handle2Set<MPI_Group>
{
public: 
  bool is_handle_null(MPI_Group handle) {
    return (handle == MPI_GROUP_NULL || handle == MPI_GROUP_EMPTY) ? 1 : 0; 
  }
} Group2Callpath; 



/************************************************
 * MPI group allocation calls 
 ************************************************/

int MPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
  int rc = PMPI_Comm_group(comm, group); 
  Group2Callpath.allocate(*group); 
  return rc; 
}

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
  int rc = PMPI_Group_union(group1, group2, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
  int rc = PMPI_Group_intersection(group1, group2, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
  int rc = PMPI_Group_difference(group1, group2, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
  int rc = PMPI_Group_incl(group, n, ranks, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
  int rc = PMPI_Group_excl(group, n, ranks, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
  int rc = PMPI_Group_range_incl(group, n, ranges, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
  int rc = PMPI_Group_range_excl(group, n, ranges, newgroup);
  Group2Callpath.allocate(*newgroup); 
  return rc; 
}

#if MPI_VERSION > 1
/************************************************
 * MPI VERSION >1 calls 
 * introduced in MPI-2.0 or later
 ************************************************/

int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
  int rc = PMPI_Comm_remote_group(comm, group);
  Group2Callpath.allocate(*group);
  return rc;
}

int MPI_Win_get_group(MPI_Win win, MPI_Group * group)
{
  int rc = PMPI_Win_get_group(win, group);
  Group2Callpath.allocate(*group);
  return rc;
}

int MPI_File_get_group(MPI_File fh, MPI_Group * group)
{
  int rc = PMPI_File_get_group(fh, group);
  Group2Callpath.allocate(*group);
  return rc;
}

#endif  

/************************************************
 * MPI group free call
 ************************************************/

int MPI_Group_free(MPI_Group *group)
{
  MPI_Group handle_copy = *group; 
  int rc = PMPI_Group_free(group); 
  Group2Callpath.free(handle_copy); 
  return rc; 
  
}
