//*****************************************************************
//
// Copyright (C) 2009 Suzanne Matthews, Tiffani Williams
//              Department of Computer Science & Engineering
//              Texas A&M University
//              Contact: [sjm,tlw]@cse.tamu.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details (www.gnu.org).
//
//*****************************************************************

#include <assert.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#define MRSRFPQ  "src/mrsrf-pq"
#define MAX_STRING_LEN 500
#define MRSRF_MPI_TEMP_DIR "mrsrf-mpi-tmp"
#define MRSRFPQ_TEMP_DIR "/tmp/mrsrf-tlw"

/* global variables -- for a particular node */
char MASTER_TREE_FILE[MAX_STRING_LEN];
int CORES;
int TAXA;
int NUM_MASTER_TREES;
int OUTPUT;
int ROUNDS;
char TAG[MAX_STRING_LEN];
int MAX_INT_32 = 32767;


int do_mkdir(const char *path, mode_t mode)
{
    struct stat st;
    int  status = 0;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist */
        if (mkdir(path, mode) != 0)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode)) // path exists, but is not a directory
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}




void process_command_line_args(int argc, char *argv[])
{
  if (argc != 7) {
    fprintf(stderr, "USAGE: %s <cores> <tree file> <taxa> <trees> <output> <rounds>\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, 1); 
  }
  CORES = atoi(argv[1]);
  strcpy(MASTER_TREE_FILE, argv[2]);
  TAXA = atoi(argv[3]);
  NUM_MASTER_TREES = atoi(argv[4]);
  OUTPUT = atoi(argv[5]);
  ROUNDS = atoi(argv[6]);
}


int main( int argc, char *argv[] )
{
  char cmd[MAX_STRING_LEN];
  char path[MAX_STRING_LEN];
  char buffer[MAX_STRING_LEN];
  char *line;
  char myname[MAX_STRING_LEN];
  char *saveptr;
  char *token;
  char treefile[MAX_STRING_LEN];
  char treefile2[MAX_STRING_LEN];
  int count;
  int i;
  int length;
  int myrank;
  int num_elems_recv;
  int num_nodes;
  int treefile_size;
  int treefile_size2;
  MPI_Status stat;
  FILE *fptr;
  struct timeval t1_start;
  struct timeval t1_end;
  double t1_time;
  struct timeval split_start;
  struct timeval split_end;
  double split_time;
  int seed = 17;
  int status;
 
  /* Track time to run mrsrf-mpi */ 
  gettimeofday(&t1_start, NULL); 
  
  /*  I initialize for myself the MPI API */
  MPI_Init(&argc, &argv);

  /*  Who am I? I request my ID number */
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  
  /* How many nodes are available */
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);

  /*  What is my hostname? */
  MPI_Get_processor_name (myname, &length);
  
  fprintf(stderr,"\n[Node %d]:****Executing MrsRF-MPI on %s**** \n",myrank, myname);
 
  /* Process command-line arguments */
  process_command_line_args(argc, argv);
  
  status = do_mkdir(MRSRFPQ_TEMP_DIR, 0777);
  if (status == -1) {
    fprintf(stderr, "Error creating directory %s", MRSRFPQ_TEMP_DIR);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
 
  /* Node 0 creates tree files for the other nodes */
  if (myrank == 0) {
    gettimeofday(&t1_start, NULL);

    // Check to make sure that the number of trees is greater than the total number of cores
    if ((CORES * num_nodes) > NUM_MASTER_TREES) { 
       fprintf(stderr, "The total number of cores (%d) is greater than the number of trees (%d) \n", CORES, NUM_MASTER_TREES);
       MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Create directory for storing files needed globally by MrsRF-MPI
    /*
    status = do_mkdir(MRSRF_MPI_TEMP_DIR, 0777);
    if (status == -1) {
      fprintf(stderr, "Error creating directory %s", MRSRF_MPI_TEMP_DIR);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    */

    //fprintf (stderr, "[Node %d]: Partitioning the input for the %d nodes\n", myrank, num_nodes);
    
    // Split input among the nodes 
    /*fprintf (stderr, "[Node %d]: Partitioning the input for the %d nodes\n", myrank, num_nodes);
    fprintf(stderr, "[Node %d]: Creating __input-0.txt\n", myrank);
    gettimeofday(&split_start, NULL);
    //sprintf(cmd, "./peanuts/split_trees.pl %s %d %d %s", MASTER_TREE_FILE, NUM_MASTER_TREES, num_nodes, "./");
    sprintf(cmd, "./peanuts/split_trees.pl %s %d %d", MASTER_TREE_FILE, NUM_MASTER_TREES, num_nodes);
    fprintf(stderr, "[Node %d]: %s\n", myrank, cmd);
    system(cmd);
    gettimeofday(&split_end, NULL);
    split_time = split_end.tv_sec - split_start.tv_sec + (split_end.tv_usec - split_start.tv_usec) / 1.e6;
    fprintf(stderr,"\n[Node %d]: ****Input split among the nodes: %g seconds**** \n", myrank, split_time);
    */
    // Create seed for all nodes to use -- necessary for calls to hashbase in MrsRF code
    srand(time(NULL));
    seed = rand() % MAX_INT_32;
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Bcast (&seed, 1, MPI_INT, 0, MPI_COMM_WORLD);

  /* Safe for all processors to continue after broadcast of seed value */
  //fprintf(stderr, "seed: %d\n", seed);
  
  // Process __input-0.txt file to get Node i's data
  /*
  sprintf(buffer, "__input-0.txt");
  fptr = fopen(buffer, "r");
  assert(fptr);
  
  line = (char *) malloc (MAX_STRING_LEN * sizeof(char));
  for (i = 0; i < num_nodes; i++) {
    if ((fgets(line, MAX_STRING_LEN - 1, fptr) != NULL) && (i == myrank)) {
      // get treefile name 
      token = strtok_r(line, " ", &saveptr);
      assert(token);
      strcpy(treefile, token);
      
      // get treefile2 name 
      token = strtok_r(NULL, " ", &saveptr);
      assert(token);
      strcpy(treefile2, token);

      // get number of trees in treefile       
      token = strtok_r(NULL, " ", &saveptr);
      assert(token);
      treefile_size = atoi(token);
      
      // get number of trees in treefile2       
      token = strtok_r(NULL, " ", &saveptr);
      assert(token);
      treefile_size2 = atoi(token);
      
      count = i;

    }
  }
  fclose(fptr);
  assert(count == myrank);
  */
  /* Run MrsRF-phoenix-ver4 */
  sprintf(cmd, "%s %d %s %d %d %d %d %d %d %d", MRSRFPQ, CORES, MASTER_TREE_FILE, TAXA, NUM_MASTER_TREES, num_nodes, myrank, OUTPUT, ROUNDS, seed);
  //sprintf(cmd, "%s %d %s %s %s %d %d %d %d %d %d %d %d", MRSRFPQ, CORES, treefile, treefile2, MASTER_TREE_FILE, TAXA, treefile_size, treefile_size2, NUM_MASTER_TREES, myrank, OUTPUT, ROUNDS, seed);
  fprintf(stderr, "[Node %d]: Running: %s\n", myrank, cmd);
  system(cmd);
  
  // Wait for all nodes to finish in order to create the final matrix
  MPI_Barrier(MPI_COMM_WORLD);

  /* node 0 constructs the final matrix */
  if (OUTPUT ==1 && myrank == 0) {  
    // Create directory for output of final RF matrix
    strcpy(path, "mrsrf-mpi-output");
    status = do_mkdir(path, 0777);
    if (status == -1) {
      fprintf(stderr, "Error creating directory %s", path);
      MPI_Abort(MPI_COMM_WORLD, 1);
    } 
        
    // Create final RF matrix
    fprintf(stderr, "[Node 0]: Creating final matrix\n");
    sprintf(cmd, "./peanuts/create_final_matrix.pl %s %d node", path, num_nodes);
    system(cmd);
    fprintf(stderr, "[Node 0]: Final matrix computed\n");
  }
 
  gettimeofday(&t1_end, NULL);
  t1_time = t1_end.tv_sec - t1_start.tv_sec + (t1_end.tv_usec - t1_start.tv_usec) / 1.e6;
  fprintf(stderr,"\n[Node %d]: ****MrsRF-MPI Completed: %g seconds**** \n", myrank, t1_time);

  /* Close the MPI API: */
  MPI_Finalize();
  
  return 0;
}
