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

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <stdarg.h>

#include "MapReduceScheduler.h"
#include "stddefines.h"
#include "mapreduce_sort.h"

#define CLUSTER_TEMP_PATH "/tmp/mrsrf-sjm"   //Writes to the temp dir on each node, partition_trees.pl creates the directory
#define MAX_KEY_LEN 100  
#define HASH_LINE_SIZE 500000   // hash table line length (used in phases 1 and 2) 
#define NAME_SIZE 100
#define BUF_SIZE 500
#define SEPARATOR -1
#define HASHBASE "hashbase/hashbase"
#define PEANUTS_DIR "peanuts"
#define MAX_INT_32 32767  // maximum 32-bit integer
#define VERBOSE 0

const char* peanut = "peanuts/";
//const char* hashbase = "hashbase/hashbase";
//const char* tempath = "tempdir/";
//const char* randomfile = "hashbase/";
int NUM_COL_TREES = 0;
int NUM_ROW_TREES = 0;
int TAXA = 0;
int * grow;
int * gcol;
unsigned short int **rf_matrix; 
char *MASTER_TREE_FILE;
char *TREE_FILE1;
char *TREE_FILE2;
int START_TREE; /* tree in map2 to begin processing */
int END_TREE;  /* tree in map2 to end processing */
int TREE_BLOCK_SIZE; /* in map2, number of trees to process at a time */
int TOTAL_ROUNDS = 1; /* number of times to iterate through map2 and reduce2 */
int CURRENT_ROUND;
int SEED_VALUE;
int NUM_NODES = 1;
int NUM_THREADS = 1;
int NUM_MASTER_TREES;
char INPUT_FILE[NAME_SIZE];
char OUTPUT_FILE[NAME_SIZE];
char FILE_EXT[NAME_SIZE] = "0";
int OUTPUT = 0;
int INPUT = 0;
int NODE_ID;
char HOME_TEMPDIR[NAME_SIZE];
char MRSRF_MPI_TEMP_DIR[50] = "mrsrf-mpi-tmp-sjm";

final_data_t mrs_rf_phase1_vals;
final_data_t mrs_rf_phase2_vals;
  
typedef struct {
    long fpos;
    long flen;
    char *fdata;
    int unit_size;
} mrs_rf_data_t;


/* Used to pass an array of values between */ 
typedef struct {
    //char *key;
    unsigned short int *val; 
} key_int_val_t;

enum {
    IN_WORD,
    NOT_IN_WORD
};

typedef struct {
	//char vals[10000];
	char *vals;
} char_vals_array_t;

typedef struct {
  int *vals;
  int count;
} int_vals_array_t;

typedef struct {
  unsigned short int *vals;
  int count;
} unsigned_short_int_vals_array_t;

typedef struct {
  unsigned short int *row_tids;
  unsigned short int *col_tids;
  unsigned short int min_row_tid;
  unsigned short int min_col_tid;
  unsigned short int num_row_tids;
  unsigned short int num_col_tids;
} row_col_tids_t;


char *getTimeString (void)
{
  struct tm *timeinfo;
  time_t rawtime;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  return asctime(timeinfo);
}


void removeNewline(char *s)
{
  int len;

  len = strlen(s);
  s[len-1] = '\0';
}


void print_msg (int verbose, const char *fmt, ...)
{
  char s[BUF_SIZE];
  time_t t;
  struct tm *ptm;
  va_list ap;

  va_start (ap, fmt);

  time(&t);
  ptm = gmtime(&t);

  strcpy(s, getTimeString());
  removeNewline(s);

  if (verbose == 1) {
    fprintf(stderr, "[Node %d]:   ", NODE_ID);
    vfprintf (stderr, fmt, ap);
    //fprintf(stderr, "\n");
    fflush(stderr);
    va_end (ap);
  }
}                 

/* Remove temporary files */
void cleanup_dir()
{
	char cmd[BUF_SIZE];
	
	//sprintf(cmd, "rm -r %s", HOME_TEMPDIR);
	sprintf(cmd, "rm -rf %s", CLUSTER_TEMP_PATH);
  system(cmd);
	//sprintf(cmd, "rm %s", CLUSTER_TEMP_PATH);
  //system(cmd);
  //system("rm -f treefile.*");
}

/* For qsort to sort numbers in ascending order */
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

/** mystrcmp()
*  Comparison function to compare 2 words
*/
int mystrcmp(const void *s1, const void *s2)
{
    //const int x = atoi((const void *)s1);
    //const int y = atoi((const void *)s2);
    //return (x < y);
    return strcmp((const char *)s1, (const char *) s2);
}

int mycmp(const void *s1, const void *s2)
{
    const int x = atoi((const void *)s1);
    const int y = atoi((const void *)s2);
    return (x < y);
}


/** mykeyvalcmp()
 *  Comparison function to compare 2 keys from phase1
 */
int mykeyvalcmp(const void *v1, const void *v2)
{
  char *token1;
  char *token2;
  char *saveptr;
  char *key1;
  char *key2;
  int i1;
  int i2;
  keyval_t* kv1 = (keyval_t*)v1;
  keyval_t* kv2 = (keyval_t*)v2;
 
  /* Make copy of keys for input to strtok_r(), which changes the input
    string in-place.
  */
  key1 = (char *) malloc (BUF_SIZE * sizeof(char));
  key2 = (char *) malloc (BUF_SIZE * sizeof(char));
  strcpy(key1, kv1->key);
  strcpy(key2, kv2->key);

  /* Parse keys to remove number of comparisons (comparisons.length of hashline).
     This is how the keys will be sorted.
  */
  saveptr = NULL;
  token1 = strtok_r(key1, ".", &saveptr);
  token2 = strtok_r(key2, ".", &saveptr);

  /* Sort keys */
  i1 = atoi(token1);
  i2 = atoi(token2);
  return (i1 < i2);
  
   //if (i1 < i2) return 1;
   //else if (i1 > i2) return -1;
   //else return 0;
   //else {
      /*** don't just return 0 immediately coz the mapreduce scheduler provides 
   1 key with multiple values to the reduce task. since the different words that 
   we require are each part of a different keyval pair, returning 0 makes
   the mapreduce scheduler think that it can just keep one key and disregard
   the rest. That's not desirable in this case. Returning 0 when the values are
   equal produces results where the same word is repeated for all the instances
   which share the same frequency. Instead, we check the word as well, and only 
   return 0 if both the value and the word match ****/
      //return strcmp((char *)kv1->key, (char *)kv2->key);
     // return 0;
   //}
}


/** mykeyvalcmp2()
*  Comparison function to compare 2 ints
*/
int mykeyvalcmp2(const void *v1, const void *v2)
{
    //keyval_t* kv1 = (keyval_t*)v1;
    //keyval_t* kv2 = (keyval_t*)v2;

    //int x = atoi((char *)kv1->key);
    //int y = atoi((char *)kv2->key);
    int x = atoi((char *)v1);
    int y = atoi((char *)v2);
    //printf (" x = %d and y =%d \n", x, y);
    if (x > y) {
        //printf("leave key comparision\n");
        return 1;
    }
    else if (x<y) {
        return -1;
    }

    return 0;
}


void print_global_hashtable(char *filename)
{
  keyval_t *curr;
  FILE *fptr;
  int i;
  int j;
  int len;
  int comparisons;
  int col_tids_starting_index;
  char *saveptr;
  char *key;
  
  key = (char *) malloc (BUF_SIZE * sizeof(char));
  print_msg(VERBOSE, "Printing global hashtable \n");
  saveptr = NULL;
  fptr = fopen(filename, "w");
  print_msg(VERBOSE, "hashtable length: %d\n", mrs_rf_phase1_vals.length);
	for (i = 0; i < mrs_rf_phase1_vals.length; i++) {
    curr = &((keyval_t *)mrs_rf_phase1_vals.data)[i];
    strcpy(key, curr->key);
    //fprintf(stderr, "key = %s\n", key);
    comparisons = atoi(strtok_r(key, ".", &saveptr));
    len = atoi(strtok_r(NULL, ".", &saveptr));
    col_tids_starting_index = atoi(strtok_r(NULL, "", &saveptr));
    fprintf(fptr, "[%d.%d.%d]: (%d)\t", comparisons, len, col_tids_starting_index, ((int *) curr->val)[0]);
    for (j = 1; j < len; j++) {
      fprintf(fptr, "%d ",  ((int *) curr->val)[j]);
	}
    fprintf(fptr, "\n");
  }
  fclose(fptr);
	free(key);
}

void print_hash_line_from_reduce1(int_vals_array_t *line)
{
  int i;
  int count;
  
  fprintf(stderr, "LINE: ");
  // Print hash line
  count = line->count;
  for (i = 0; i < count; i++) {
    fprintf(stderr, "%d ", line->vals[i]);
  }
  fprintf(stderr, "\n");
}

void print_local_hashtable_map2(unsigned short int **hashtable, int *lengthOf, int *start_col_tids, int core_id, int num_hash_lines)
{
  int x;
  int y;
  char s[BUF_SIZE];
  FILE *fptr;

  sprintf(s, "hashtable-%d-%d.out", NODE_ID, core_id);
  fptr = fopen(s, "w");
  fprintf(fptr, "Printing hashtable \n");
  for (x = 0; x < num_hash_lines; x++) {
    fprintf(fptr, "[%d]<%d>(%d): ", hashtable[x][0], start_col_tids[x], lengthOf[x]);
      for ( y = 0; y < lengthOf[x]; ++y) {
        fprintf(fptr, "%d ", hashtable[x][y]);
      }
      fprintf(fptr, "\n");
    }
    fclose(fptr); 
}

/** mrs_rf_splitter()
*  Memory map the file and divide file on a tree border i.e. a newline.
*/
int mrs_rf_splitter(void *data_in, int req_units, map_args_t *out)
{
    mrs_rf_data_t * data = (mrs_rf_data_t *)data_in;

    assert(data_in);
    assert(out);

    assert(data->flen >= 0);
    assert(data->fdata);
    assert(req_units);
    assert(data->fpos >= 0);

    // End of file reached, return FALSE for no more data
    if (data->fpos >= data->flen) return 0;

    // Set the start of the next data
    out->data = (void *)&data->fdata[data->fpos];

    // Determine the nominal length
    //out->length = req_units * data->unit_size;
    out->length = 0;


    if (data->fpos + out->length > data->flen)
        out->length = data->flen - data->fpos;

    // Set the length to end at a newline
    for (data->fpos += (long)out->length;
        data->fpos < data->flen &&
            //data->fdata[data->fpos] != ' ' && data->fdata[data->fpos] != '\t' &&
            //data->fdata[data->fpos] != '\r' &&
        data->fdata[data->fpos] != '\n';
        data->fpos++, out->length++);
    data->fpos++;
    out->length++;
    return 1;
}

/* Write out a file*/
void print_hashbase_output(FILE *inputfile, char *output)
{
  //char line[HASH_LINE_SIZE];
  char *line;
  FILE *fp;
	
  line = (char *) malloc (HASH_LINE_SIZE * sizeof(char));
  fp = fopen(output, "w");
  while (fgets(line, HASH_LINE_SIZE - 1, inputfile) != NULL) {
  	fprintf(fp, "%s", line);
  }
  fclose(fp);
	
	free(line);
}

void compute_number_row_col_trees(char *filename)
{
	char line[BUF_SIZE];
	char *token;
	char *saveptr;
	FILE *fp;
	
	fp = fopen(filename, "r");
	/* There is only one line in this file. The line has the
	   format NUM_ROW_TREES x NUM_COL_TREES.
	*/
	fgets(line, BUF_SIZE - 1,  fp); // there is only one line in this file

	// determine number of row trees
	token = strtok_r(line, " ", &saveptr);
	assert(token);
	NUM_ROW_TREES = atoi(token);
	print_msg(VERBOSE, "NUM_ROW_TREES = %d\n", NUM_ROW_TREES);
	
	// discard the 'x'
	token = strtok_r(NULL, " ", &saveptr);
	assert(token);
	
	// determine number of col trees
	token = strtok_r(NULL, " ", &saveptr);
	assert(token);
	NUM_COL_TREES = atoi(token);
	print_msg(VERBOSE, "NUM_COL_TREES = %d\n", NUM_COL_TREES);
	
  TREE_BLOCK_SIZE = NUM_ROW_TREES/TOTAL_ROUNDS;
	print_msg(VERBOSE, "TREE_BLOCK_SIZE = %d\n", TREE_BLOCK_SIZE);
}

int count_tids (char *s)
{
  int tree_count = 0;
	int i;
	
  for (i = 0; i < strlen(s); i++) {
    if (s[i] == ' ') {
        tree_count++;
    }
	}
	return tree_count;
}

void process_hashbase_output(FILE *inputfile, int multiplier)
{
  int i;
  char *token;
  char *token1;
  char *token2;
  char *token3;
  char *line; 
  char *saveptr;
  char *temp;
  char *mykey;
  int_vals_array_t *list_of_tids;
	int hash_line_tids;
	int max_trees;
   
	if (multiplier == 1) {
		max_trees = NUM_ROW_TREES;
	}
	else {
		max_trees = NUM_COL_TREES;
	}
	print_msg(VERBOSE, "max_trees: %d \n", max_trees);
   
  line = (char *) malloc (HASH_LINE_SIZE * sizeof(char));
  assert(line);

  //mykey = (char *) malloc (MAX_KEY_LEN * sizeof(char));
  //assert(mykey);
   
  while (fgets(line, HASH_LINE_SIZE - 1, inputfile) != NULL) { 
		

		token1 = strtok_r(line, ":", &saveptr); //LINE: (ignore)
		if (token1 == NULL) {
		  fprintf(stderr, "line is: %s", line);
		  fprintf(stderr, "ERROR token1 is NULL\n");
		  exit(1);
		}  
		//fprintf(stderr, "token1: %s\n", token1);

		token2 =  strtok_r(NULL, ":", &saveptr); //bipartition id: 
	  if (token2 == NULL) {
		  fprintf(stderr, "ERROR token2 is NULL\n");
		  fprintf(stderr, "line is: %s", line);
		  fprintf(stderr, "token1 is: %s", token1);
		  exit(1);
		}
		mykey = (char *) malloc (MAX_KEY_LEN * sizeof (char));
		strcpy(mykey,token2);
		//fprintf(stderr, "token2: %s\n", token2);
     
		token3 = strtok_r(NULL, ":", &saveptr); //list of tree ids
		if (token3 == NULL) {
		  fprintf(stderr, "ERROR token3 is NULL\n");
		  fprintf(stderr, "line is: %s", line);
		  fprintf(stderr, "token1 is: %s", token1);
		  fprintf(stderr, "token2 is: %s", token2);
		  exit(1);
		}  
		//fprintf(stderr, "token3: %s", token3);
		temp = strtok_r(token3, "\n", &saveptr); // remove newline from list of tids
		assert(temp); 
		//fprintf(stderr, "tokens assigned \n");
		
		
		/* For each line, allocate storage for the key
		   and values (tids)
		*/
		//fprintf(stderr, "%s", line);
		list_of_tids = (int_vals_array_t *) malloc (1 * sizeof(int_vals_array_t));
		hash_line_tids = count_tids(token3);
    list_of_tids->vals = (int *) malloc ((hash_line_tids + 1) * sizeof(int));
    //list_of_tids->vals = (int *) malloc ((max_trees + 1) * sizeof(int));
		assert(list_of_tids->vals);	
		
		// convert remaining hash line of tids into an into an integer array
		//fprintf(stderr, "convert to int: ");
		i = 1;
		token = strtok_r(temp, " ", &saveptr);
		while (token != NULL) {
			//fprintf(stderr, "%s ", token);
		  list_of_tids->vals[i] = atoi(token);
		  //list_of_tids->vals[i] = atoi(token) - offset;
		  //fprintf(stderr, "[%d] ", list_of_tids->vals[i]);
		  token = strtok_r(NULL, " ", &saveptr);
		  i++;
		}
		list_of_tids->vals[0] = multiplier;
		//fprintf(stderr, "\n");
		list_of_tids->count = i; /* number of tids in this bipartition */
		//fprintf(stderr, "%d\n", list_of_tids->count);

		assert((list_of_tids->count > 0) && (list_of_tids->count <= (max_trees +1)));
		
		/*
		token = strtok_r(mykey, ".", &saveptr);
		token2 = strtok_r(NULL,"\n", &saveptr);
		buffer = (char *) malloc (MAX_KEY_LEN * sizeof(char));
		sprintf(buffer, "%d.%d", atoi(token2), atoi(token));
		//fprintf(stderr, "map key = %s\n", buffer);
		*/
		emit_intermediate((void *) mykey, (void *) list_of_tids, strlen(mykey));
		//fprintf(stderr, "EMIT_LINE: ");
		for (i = 0; i < list_of_tids->count; i++) {
		  //fprintf(stderr, "%d ", list_of_tids->vals[i]);
		}
		//fprintf(stderr, "\n");
  }
	free(line);
}

void mrs_rf_map1(map_args_t *args) 
{
  char *curr_start;// curr_ltr;
  int state = NOT_IN_WORD;
  int i;
  char * my_data = (char*)malloc(NAME_SIZE * sizeof(char));  
  char *data = (char *)args->data;
  char *token;
  FILE *inputfile;
  char *saveptr;
  int offset1; //for proper labelling of tree ids
  char * myfile1 = (char*)malloc(NAME_SIZE*sizeof(char));
  char * cmd = (char*)malloc(NAME_SIZE*sizeof(char));
  int myfile1_size;
	int multiplier;
  assert(args);
   
  //snprintf(trees, NAME_SIZE,"%d", TREES);

  //fprintf(stderr, "inside phase1_map\n");

  assert(data);
  curr_start = data;
  strncpy(my_data, data, args->length);
  saveptr = NULL;

  /* Obtain appropriate data which was parsed from input-*.txt*/
  token = strtok_r(my_data, " ", &saveptr);
  myfile1 = token;
   
  token = strtok_r(NULL, " ", &saveptr);
  myfile1_size = atoi(token);
 
  token = strtok_r(NULL, " ", &saveptr);
  offset1 = atoi(token);
   
  token = strtok_r(NULL, " ", &saveptr);
  multiplier = atoi(token);
	
  print_msg(VERBOSE, "myfile1 = %s\n", myfile1);
  print_msg(VERBOSE, "myfile1_size = %d\n", myfile1_size);
  print_msg(VERBOSE, "offset1 = %d\n", offset1);
  print_msg(VERBOSE, "multiplier = %d\n", multiplier);
   
  // Execute hashbase on treefile 1 and write output to file
  /*
  sprintf(cmd,  "%s %s/%s %s %d %d %d -s %d", HASHBASE, CLUSTER_TEMP_PATH, myfile1, MASTER_TREE_FILE, myfile1_size, NUM_MASTER_TREES, offset1, SEED_VALUE);
  print_msg(VERBOSE, "HashBase command for treefile1: %s\n", cmd);
  inputfile = popen(cmd, "r");
  assert(inputfile);
  sprintf(buffer, "hashbase.output.%d", NODE_ID);
  print_hashbase_output(inputfile, buffer); 
  pclose(inputfile);
  */
  
  // Execute hashbase on treefile 1
  sprintf(cmd,  "%s %s/%s %s %d %d %d -s %d", HASHBASE, CLUSTER_TEMP_PATH, myfile1, MASTER_TREE_FILE, myfile1_size, NUM_MASTER_TREES, offset1, SEED_VALUE);
  print_msg(VERBOSE, "HashBase command for treefile1: %s\n", cmd);
  inputfile = popen(cmd, "r");
  assert(inputfile);
  //process_hashbase_output(inputfile, "row", NUM_ROW_TREES);
  process_hashbase_output(inputfile, multiplier);
  pclose(inputfile);

  // Execute hashbase on treefile 2
	/*
  sprintf(cmd,  "%s %s/%s %s %d %d %d -s %d", HASHBASE, CLUSTER_TEMP_PATH, myfile2, MASTER_TREE_FILE, myfile2_size, NUM_MASTER_TREES, offset2, SEED_VALUE);
  print_msg(VERBOSE, "HashBase command for treefile1: %s\n", NODE_ID, cmd);
  inputfile = popen(cmd, "r");
  assert(inputfile);
  process_hashbase_output(inputfile, "col", NUM_COL_TREES);
  pclose(inputfile);
	*/
	
  // THIS IS ESSENTIAL.  LEAVE ALONE!!!
  i = (int)args->length;
  if (state == IN_WORD) {
    data[args->length] = 0;
  }
	//free(my_data);
	free(myfile1);
	free(cmd);
  //fprintf(stderr, "Finished with Map phase 1! \n");
}

/** mrs_rf_reduce1()
 * Add up the partial sums for each word
 */
void mrs_rf_reduce1(void *key_in, void **vals_in, int vals_len) 
{
  int_vals_array_t *list_of_tids;
  int_vals_array_t **vals = (int_vals_array_t **) vals_in; 
  int i;
  int j;
  int row_tids; 
  int col_tids; 
  int col_index;
  int row_index;
  int min_row;
  int value;
  int total_comparisons;
  char *buffer;
  
  assert(key_in);
  assert(vals);
 
  /* count the number of row and col tids in this bipartition */
  row_tids = 0;
  col_tids = 0;
  for (i = 0; i < vals_len; i++) {
    //fprintf(stderr, "vals[%d]->vals[0]: %d\n", i, vals[i]->vals[0]);
    //fprintf(stderr, "vals[%d]->count: %d\n", i, vals[i]->count);
    if(vals[i]->vals[0] == 1){
      row_tids += (vals[i]->count - 1); // don't count vals[0]
    }
    else {
      col_tids += (vals[i]->count - 1); // don't count vals[0]
    }
  }
  //print_msg(VERBOSE, "row_tids: %d\n", row_tids);
  //print_msg(VERBOSE, "col_tids: %d\n", col_tids);
  total_comparisons = row_tids * col_tids;
  //fprintf(stderr, "total comparisons: %d\n\n", total_comparisons);

  // Print number of row and col tids   
  //fprintf(stderr, "BID: %s: (col_tids, row_tids): (%d, %d) \n", (char *) key_in, col_tids, row_tids);
  assert((row_tids <= NUM_ROW_TREES) && (col_tids <= NUM_COL_TREES));

  // Only process bipartition if there are row and col tids  
  if (total_comparisons > 0) {
    list_of_tids = (int_vals_array_t *) malloc (sizeof(int_vals_array_t));
    assert(list_of_tids); 
    
    // +1 because min row tid will be in [0]  
    list_of_tids->vals = (int *) malloc ((row_tids + col_tids + 1) * sizeof(int));
    assert(list_of_tids->vals);

    /* Fill list_of_tids-> vals with row_tids followed by col_tids. The first
       row tid wil be in location 1 of the array.  The first col tid will be
       at the end of the array.  The row tids will be inserted in the array to
       to the right.  While the col tids will be inserted in the array to the left
       of the previosly inserted col tid.  The smallest row tid will be placed
       into location 0 of the array.
    */
    row_index = 1; 
    col_index = row_tids + col_tids;
    min_row = USHRT_MAX;
    //fprintf(stderr, "USHRT_MAX: %d\n", min_row);
    for (i = 0; i < vals_len; i++) {
      if (vals[i]->vals[0] == 1) {
        for (j = 1; j < vals[i]->count; j++) {
          //fprintf(stderr, "%d ", vals[i]->vals[j]);
          value = vals[i]->vals[j];
          list_of_tids->vals[row_index] = value;
          row_index++;
	
    	    if ((value <= min_row) && (value >= 0)) {
            min_row = value;
          }
        }  
      }
      else{
        for (j = 1; j < vals[i]->count; j++) {
          //fprintf(stderr, "%d ", vals[i]->vals[j]);
          value = vals[i]->vals[j];
          list_of_tids->vals[col_index] = value;
          col_index--;
        } 
      }
    }
    //fprintf(stderr, "row_index: %d\n", row_index);
    //fprintf(stderr, "col_index: %d\n", col_index);
    assert((row_index - 1) == col_index); // account for row_index start at 1
    
    list_of_tids->vals[0] = min_row;
    list_of_tids->count = row_tids + col_tids + 1;
    buffer = (char *) malloc (BUF_SIZE * sizeof(char));
    // row_tids + 1 gives the index for the start of col_tids
    sprintf(buffer, "%d.%d.%d", total_comparisons, list_of_tids->count, row_tids+1);
    //fprintf(stderr, "key: %s\n", buffer);
    //print_hash_line_from_reduce1(list_of_tids);
    // Sort row tids
    qsort(list_of_tids->vals+1, row_tids, sizeof(int), compare);
    // Sort col tids
    qsort(list_of_tids->vals+col_index+1, col_tids, sizeof(int), compare);
    //print_hash_line_from_reduce1(list_of_tids);
    emit(buffer, (void *) list_of_tids->vals);
  }
  
  // Deallocate vals array
  for (i = 0; i < vals_len; i++) {
    free (vals[i]);
  }
  //free (vals);
}


/* Partitioner for phase 2 of Mrs. RF.  Assigns keys within
   a range to a reducer.
*/
int mrs_rf_partition2(int reduce_tasks, void* key, int key_size)
{
    int reduce_id = atoi(key) / (int)(NUM_ROW_TREES/NUM_THREADS);
    //fprintf(stderr, "key = %d, reduce_id = %d\n", atoi(key), reduce_id);
    return (reduce_id);
}


/** mrs_rf_map2()
  * Go through the allocated portion of the file and map the bipartitions
*/
void mrs_rf_map2(map_args_t *args)
{
  char *curr_start;// curr_ltr;
  int state = NOT_IN_WORD;
  int i;
  int j;
  int x;
  int y;
  unsigned short int elem;
  char * my_data = (char*)malloc(BUF_SIZE * sizeof(char));
  char *data = (char *)args->data;
  char *token;
  char *saveptr = NULL;
  unsigned short int  ** hashtable;
  int min = INT_MAX;
  int *lengthOf;
  int *min_loc;
  int *start_col_tids;
  struct timeval t1_start;
  struct timeval t1_end;
  double t1_time;
  int core_id;
  int hash_loc;
  int start_hash_loc;
  int end_hash_loc;
  int num_hash_lines;
  int len;
  keyval_t *curr;
  int index;

  assert(args);
  assert(data);
  curr_start = data;
  strncpy(my_data, data, args->length);  // my_data is the assigned string from input-prep.txt

  token = strtok_r(my_data, " ", &saveptr);
  core_id = atoi(token);
  print_msg(VERBOSE, "core_id is %d\n", core_id);
    
  token = strtok_r(NULL, " ", &saveptr);
  start_hash_loc = atoi(token);
  print_msg(VERBOSE, "start_hash_loc is %d\n", start_hash_loc);

  token = strtok_r(NULL, " ", &saveptr);
  num_hash_lines = atoi(token);        
  print_msg(VERBOSE, "num_hash_lines is %d\n", num_hash_lines);

  //num_bipart = mrs_rf_phase1_vals.length;
  //fprintf(stderr, "num_bipart = %d\n", mrs_rf_phase1_vals.length);

  hashtable = (unsigned short int **) malloc(num_hash_lines * sizeof(unsigned short int *));
  assert(hashtable);
    
  lengthOf = (int*)malloc(num_hash_lines * sizeof(int));
  assert(lengthOf);
  
  start_col_tids = (int*)malloc(num_hash_lines * sizeof(int));
  assert(start_col_tids);
  
  min_loc = (int*)malloc(num_hash_lines * sizeof(int));
  assert(min_loc);

	/* range of tree ids for this mapper is from start_tree to end_tree */
  end_hash_loc = start_hash_loc + num_hash_lines - 1;

  /* Track time to create hash table */ 
  gettimeofday(&t1_start, NULL);
    
  index = 0;
  unsigned int min2;
  hash_loc = core_id;
  for (i = 0; i < num_hash_lines; i++) {
    min2 = INT_MAX;
   	curr = &((keyval_t *)mrs_rf_phase1_vals.data)[hash_loc];
   	//fprintf(stderr, "HASH_LINE: [%s]  ", (char *) curr->key);
   	token = strtok_r(curr->key, ".", &saveptr); // number of comparisons
   	len = atoi(strtok_r(NULL, ".", &saveptr)); // hash line length
   	token = strtok_r(NULL, "", &saveptr); // number of row tids
    lengthOf[index] = len;
    start_col_tids[index] = atoi(token); // works because [0] is where min is stored
    min_loc[index] = 1;
    
   	//fprintf(stderr, "lengthOf[%d]= %d \n", i, len);
   	hashtable[index] = (unsigned short int *) malloc (len * sizeof(unsigned short int));
   	assert(hashtable[index]);
   	for (j = 0; j < len; j++) {
   		hashtable[index][j] = (unsigned short int) ((int *) curr->val)[j];
      //fprintf(stderr, "%d ", hashtable[index][j]);
  	}
  	index++;
   	hash_loc += NUM_THREADS;
  }
  assert(index == num_hash_lines);
  
  /* hash table creation time */
  gettimeofday(&t1_end, NULL);
  t1_time = t1_end.tv_sec - t1_start.tv_sec + (t1_end.tv_usec - t1_start.tv_usec) / 1.e6;
  print_msg(1, "****Hash table creation time: %g seconds**** \n", t1_time);

  // <----- Hash table file creation done ----> 
 
  //print_local_hashtable_map2(hashtable, lengthOf, start_col_tids, core_id, num_hash_lines);

  // Process hash table 
    
  gettimeofday(&t1_start, NULL);
    
  //fprintf(stderr, "hash table read from file\n");
    
  //compute row vector
  //fprintf(stderr, "Processing hash table ... \n");
  min = INT_MAX;
  //fprintf(stderr, "START_TREE: %d \n", START_TREE);
  //fprintf(stderr, "END_TREE: %d\n", END_TREE);
  for (i = START_TREE; i < END_TREE; i++) {
    //print_msg(VERBOSE, "*******TREE: %d ***********\n", i);
    key_int_val_t keyval;
    keyval.val = (unsigned short int *) malloc (NUM_COL_TREES  * sizeof(unsigned short int));
    assert(keyval.val);
      
    //Initialize col
    for (x = 0; x < NUM_COL_TREES; x++) {
      keyval.val[x] = 0;
    }                

    for (x = 0; x < num_hash_lines; x++) {
      //fprintf(stderr, "lengthOf[%d] = %d\n", x, lengthOf[x]);
      //fprintf(stderr, "hashtable[%d][0]: %d, min_loc = %d, min = %d \n", x, hashtable[x][0], min_loc[x], hashtable[x][min_loc[x]]);
      if ((hashtable[x][0] == i) && (min_loc[x] < start_col_tids[x])) {
        for (y = start_col_tids[x]; y < lengthOf[x]; y++) {
          //fprintf(stderr, "%d ", x, y, hashtable[x][y]);
          elem = hashtable[x][y];
          keyval.val[elem]++; 
        }
        // Since line is sorted, next min is in min_loc[x] + 1
        min_loc[x] = min_loc[x] + 1;
        hashtable[x][0] = hashtable[x][min_loc[x]];
      }
    }
      
    // Emit key,value pair
    //fprintf(stderr, "Emit for map2\n");
    char * mykey = (char *) malloc (MAX_KEY_LEN*sizeof(char));
    sprintf(mykey,"%d", i % TREE_BLOCK_SIZE);
    //fprintf(stderr, "mykey: %s\n", mykey);
    emit_intermediate(mykey, (void *) keyval.val, sizeof(mykey));
  }
  
  // Check that the row_tids have been processed correctly
  /*
  for (i = 0; i < num_hash_lines; i++) {
    //assert((min_loc[i] - 1) == start_col_tids[i]);
    //fprintf(stderr, "(min_loc[%d], start_col_tids[%d]): (%d, %d) \n", i, i, min_loc[i], start_col_tids[i]);
  }
  */

  /* hash table processing time */
  gettimeofday(&t1_end, NULL);
  t1_time = t1_end.tv_sec - t1_start.tv_sec + (t1_end.tv_usec - t1_start.tv_usec) / 1.e6;
  //fprintf(stderr, "counter = %d\n", counter);
  print_msg(1, "****Hash table processing time: %g seconds**** \n", t1_time);
    
  //fprintf(stderr, "before essential part \n");

  // THIS IS ESSENTIAL.  LEAVE ALONE!!!
  i = (int)args->length;
  // For proper splitting purposes --> keep at all costs
  if (state == IN_WORD) {
    data[args->length] = 0;
    //emit_intermediate(curr_start, (void *)1, &data[i] - curr_start + 1);
  }
  //printf("i is: %d\n", i);

  // Deallocate arrays  
  //fprintf(stderr,"Deallocating hashtable ... \n");
  for (i = 0; i < num_hash_lines; i++) {
    free(hashtable[i]);
  }
  free(hashtable);
  free(lengthOf);
  free(start_col_tids);
  free(min_loc);
  //fprintf(stderr,"REDUCE phase 2\n");
}


/** mrs_rf_reduce2()
  * Add up the partial sums for each word
*/
void mrs_rf_reduce2(void *key_in, void **vals_in, int vals_len)
{
    char * key = (char *)key_in;
    unsigned short int **keyval = (unsigned short int **) vals_in;
    int x;
    int y;
    unsigned short int element;

    // Process array
    unsigned short int *sum;
    sum = (unsigned short int *) malloc (NUM_COL_TREES * sizeof(unsigned short int));
        
    for (x = 0; x < NUM_COL_TREES; x++) {
      sum[x] = 0;
    }

    for (x = 0; x < vals_len; x++) {
      //assert(atoi(key) == atoi(keyval[x]->key));
      for (y = 0; y < NUM_COL_TREES; y++) {
        element = keyval[x][y];
        // element = keyval[y];
        sum[y] += element;
        //free(keyval[x]->val[y]);
      }
      //keyval[x]->key = NULL;
      //free(keyval[x]->val);
      //free(keyval[x]->key);
      //free(keyval[x]);
    }

    // Print out sum
    /*fprintf(stderr, "key %d: ", atoi(key));
    for (x = 0; x < TREES; x++) {
      fprintf(stderr, "%d ", sum[x]);
    }
    fprintf(stderr, "\n");
    */

    rf_matrix[atoi(key)] = sum;
    //free(keyval);
    //fprintf(stderr, "count = %d\n", count);
    //count++;
}



int main(int argc, char *argv[]) 
{
    
  int i;
  int j;
  int k;
  int fd;
  int output;
  int round;
  int status;
  char * fdata;
  struct stat finfo;
  char * cmd = (char *)malloc(BUF_SIZE);
  FILE *fout = NULL;
  char buffer[BUF_SIZE];
	
  struct timeval prog_start;
  struct timeval prog_end;
  struct timeval phase1_start;
  struct timeval phase1_end;
  struct timeval phase2_start;
  struct timeval phase2_end;
  struct timeval merge_start;
  struct timeval merge_end;
  struct timeval split_input_start;
  struct timeval split_input_end;
  struct timeval print_start;
  struct timeval print_end;
  double prog_time = 0.0; 
  double phase1_time = 0.0;
  double phase2_time = 0.0; 
  double merge_time = 0.0; 
  double split_input_time = 0.0;
  double print_time = 0.0;

  struct stat st;

  gettimeofday(&prog_start, NULL); 
 
  // Remove any unnecessary temporary files 
  cleanup_dir();

  // Process command-line arguments 
  //process_command_line_args(argc, argv);
  if (argc != 10) {
    print_msg(1, "USAGE: %s <cores> <master tree file> <taxa> <master tree file size> <nodes> <node id> <output> <rounds> <seed> \n", argv[0]);
    exit(1);
  }
    
  NUM_THREADS = atoi(argv[1]);
  //TREE_FILE1 = argv[2]; //tree file 1
  //TREE_FILE2 = argv[3]; //tree file 1
	MASTER_TREE_FILE = argv[2];
  TAXA = atoi(argv[3]); //taxa
  //NUM_ROW_TREES = atoi(argv[6]);
  //NUM_COL_TREES = atoi(argv[7]);
	NUM_MASTER_TREES = atoi(argv[4]);
	NUM_NODES = atoi(argv[5]);
  NODE_ID = atoi(argv[6]);
  output = atoi(argv[7]); // print the RF matrix to file?
  TOTAL_ROUNDS = atoi(argv[8]); // minimizes the amount of memory used
  SEED_VALUE = atoi(argv[9]);
    
    
  /*************************************************************************/
  /**************************** PHASE 1 ************************************/ 
  /*************************************************************************/

  // Start time for Phase 1 
  gettimeofday(&phase1_start, NULL);

  print_msg(1,"*********Running MrsRF-pq on Node%d*********\n", NODE_ID);
  print_msg(VERBOSE, "Command-line args: \n");
  //assert(INPUT_FILE);
  print_msg(VERBOSE, "   number of cores: %d\n", NUM_THREADS);
  //print_msg(VERBOSE, "   tree file1: %s\n", TREE_FILE1);
  //print_msg(VERBOSE, "   tree file2: %s\n", TREE_FILE2);
	print_msg(VERBOSE, "   master tree file: %s\n", MASTER_TREE_FILE);
  print_msg(VERBOSE, "   number of taxa: %d\n", TAXA);
  //print_msg(VERBOSE, "   number of trees in tree file1: %d\n", NUM_ROW_TREES);
  //print_msg(VERBOSE, "   number of trees in tree file2: %d\n", NUM_COL_TREES);
	print_msg(VERBOSE, "   number of trees in master file: %d\n", NUM_MASTER_TREES);
  print_msg(VERBOSE, "   node id: %d\n", NODE_ID);
  print_msg(VERBOSE, "   output (0 = no, 1 = yes): %d\n", OUTPUT);
  print_msg(VERBOSE, "   number of rounds: %d \n", TOTAL_ROUNDS);
  print_msg(VERBOSE, "   seed: %d \n", SEED_VALUE);

  print_msg(1, "************************MrsRF-pq: Running Phase 1...*******************\n");
  print_msg(1, "Splitting tree file into %d files..\n", NUM_THREADS);
  
  sprintf(HOME_TEMPDIR, "tempdir-%d", NODE_ID);
  //fprintf(stderr, "HOME_TEMPDIR: %s\n", HOME_TEMPDIR);
	
  // Split input file into NUM_THREADS files 
  gettimeofday(&split_input_start, NULL);
  //sprintf(cmd, "./%s/split_trees2.pl %s %s %d %d %d %s", PEANUTS_DIR, TREE_FILE1, TREE_FILE2, NUM_ROW_TREES, NUM_COL_TREES, NUM_THREADS, CLUSTER_TEMP_PATH);
  sprintf(cmd, "./%s/partition_trees.pl %s %d %d %d %d %s", PEANUTS_DIR, MASTER_TREE_FILE, NUM_MASTER_TREES, NUM_NODES, NODE_ID, NUM_THREADS, CLUSTER_TEMP_PATH);
  status = system(cmd);
  gettimeofday(&split_input_end, NULL);
  split_input_time = split_input_end.tv_sec - split_input_start.tv_sec + (split_input_end.tv_usec - split_input_start.tv_usec) / 1.e6;
  print_msg(1, "****Split input time: %g seconds**** \n", split_input_time);
  
	
	// Read file node-*.size to determine the size of the p x q matrix for this node.
	sprintf(buffer, "%s/node-%d.size", CLUSTER_TEMP_PATH, NODE_ID);
	compute_number_row_col_trees(buffer);
	
  // Read in the file that partitions the trees among the cores
  sprintf(buffer, "%s/input-%d.txt", CLUSTER_TEMP_PATH, NODE_ID);
  //fprintf(stderr, "File to open: %s\n", buffer);
  CHECK_ERROR((fd = open(buffer, O_RDONLY)) < 0);
  // Get the file info (for file length)
  CHECK_ERROR(fstat(fd, &finfo) < 0);
  // Memory map the file
  CHECK_ERROR((fdata = mmap(0, finfo.st_size + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == NULL);
   
  // Setup splitter args
  mrs_rf_data_t mrs_rf_data;
  mrs_rf_data.unit_size = 5; // approx 5 bytes per word
  mrs_rf_data.fpos = 0;
  mrs_rf_data.flen = finfo.st_size;
  mrs_rf_data.fdata = fdata;

  // Setup scheduler args
  scheduler_args_t sched_args;
  memset(&sched_args, 0, sizeof(scheduler_args_t));
  sched_args.task_data = &mrs_rf_data;
  sched_args.map = mrs_rf_map1;
  sched_args.reduce = mrs_rf_reduce1;
  sched_args.splitter = mrs_rf_splitter;
  sched_args.key_cmp = mystrcmp;
  sched_args.unit_size = mrs_rf_data.unit_size;
  sched_args.partition = NULL; // use default
  sched_args.result = &mrs_rf_phase1_vals;
  sched_args.data_size = finfo.st_size;
   
  sched_args.num_map_threads = NUM_THREADS;
  sched_args.num_reduce_threads = 1;
  sched_args.num_merge_threads = NUM_THREADS;
  sched_args.num_procs = NUM_THREADS;
  
  printf("Mrs. RF: Running Phase 1...\n");
  
  CHECK_ERROR(map_reduce_scheduler(&sched_args) < 0);

  // Unmap input-*.txt (input-nodeid.txt) 
  CHECK_ERROR(munmap(fdata, finfo.st_size + 1) < 0);
  CHECK_ERROR(close(fd) < 0);
  
  print_msg(1, "Global hash table constructed.\n");
  print_msg(1, "Number of bipartitions in hash table: %d\n", mrs_rf_phase1_vals.length); 
   
  //print_global_hashtable("unsorted-hashtable.txt");
   
  // Sort bipartitions by decreasing order of comparisons
  gettimeofday(&merge_start, NULL);
  print_msg(1, "Sorting bipartitions by number of comparisons required\n");
  qsort(mrs_rf_phase1_vals.data, mrs_rf_phase1_vals.length, sizeof(keyval_t), mykeyvalcmp);
  //mapreduce_sort(mrs_rf_phase1_vals.data, mrs_rf_phase1_vals.length, sizeof(keyval_t), mykeyvalcmp, NUM_THREADS);
  //sprintf(buffer, "sorted-hashtable-%d.txt", NODE_ID);
  //print_global_hashtable(buffer);
  
  // Start time for merging bipartitions 
  //gettimeofday(&merge_start, NULL);
  sprintf(cmd, "./%s/phase2_prep.pl %d %d %s", PEANUTS_DIR, NUM_THREADS, mrs_rf_phase1_vals.length, CLUSTER_TEMP_PATH);
  //fprintf(stderr, "%s\n", cmd);
  status = system(cmd);

  /* merging execution time */
  gettimeofday(&merge_end, NULL);
  merge_time = merge_end.tv_sec - merge_start.tv_sec + (merge_end.tv_usec - merge_start.tv_usec) / 1.e6;
  print_msg(1,"****Sort time: %g seconds**** \n", merge_time);
  //print_msg(1,"****Merge time: %g seconds**** \n", merge_time);
  
  /* Get phase 1 execution time */
  gettimeofday(&phase1_end, NULL);
  phase1_time = phase1_end.tv_sec - phase1_start.tv_sec +
  (phase1_end.tv_usec - phase1_start.tv_usec) / 1.e6;
  print_msg(1,"*******Phase 1 done: %g seconds******* \n\n", phase1_time);

  /*************************************************************************/
  /**************************** PHASE 2 ************************************/ 
  /*************************************************************************/
    
  print_msg(1, "**********************MrsRF-pq: Running Phase 2...****************************\n");

    
  // Start time for Phase 2 
  gettimeofday(&phase2_start, NULL);
    
  print_msg(VERBOSE, "Total number of rounds: %d\n", TOTAL_ROUNDS);
    
  for (round = 0; round < TOTAL_ROUNDS; round++) {
    print_msg(VERBOSE, "****** ROUND %d *******\n", round+1);

    //CURRENT_ROUND = round;
      
    // Allocate RF matrix to hold result
    rf_matrix = (unsigned short int **) malloc (TREE_BLOCK_SIZE * sizeof(unsigned short int *));

    /* tree ids to process in map2 */      
    START_TREE = TREE_BLOCK_SIZE * round;
    END_TREE = TREE_BLOCK_SIZE * (round + 1);
		print_msg(VERBOSE, "START_TREE = %d\n", START_TREE);
		print_msg(VERBOSE, "END_TREE = %d\n", END_TREE);
    //fprintf(stderr, "Main thread: START_TREE = %d\n", START_TREE);
    //fprintf(stderr, "Main thread: END_TREE = %d\n", END_TREE);
            
    /* Read in the file */
    sprintf(buffer, "%s/input-prep.txt", CLUSTER_TEMP_PATH);
    //fprintf(stderr, "File to open: %s\n", buffer);
    CHECK_ERROR((fd = open(buffer, O_RDONLY)) < 0);
            
    // Get the file info (for file length)
    CHECK_ERROR(fstat(fd, &finfo) < 0);
      
    // Memory map the file
    CHECK_ERROR((fdata = mmap(0, finfo.st_size + 1,	PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == NULL);

    /* Setup scheduler args for phase 2.  This might be overkill
	 since values from phase 1 probably could have been reused.
      */ 
      
    // Setup splitter args
    mrs_rf_data_t mrs_rf_data2;
    mrs_rf_data2.unit_size = 5; // approx 5 bytes per word
    mrs_rf_data2.fpos = 0;
    mrs_rf_data2.flen = finfo.st_size;
    mrs_rf_data2.fdata = fdata;
      
    // Setup scheduler args 
    scheduler_args_t sched_args2;
    memset(&sched_args2, 0, sizeof(scheduler_args_t));
    sched_args2.task_data = &mrs_rf_data2;
    sched_args2.map = mrs_rf_map2;
    sched_args2.reduce = mrs_rf_reduce2;
    sched_args2.splitter = mrs_rf_splitter;
    sched_args2.key_cmp = mykeyvalcmp2;
    sched_args2.unit_size = mrs_rf_data.unit_size;
    sched_args2.partition = mrs_rf_partition2; // use default
    sched_args2.result = &mrs_rf_phase2_vals;
    sched_args2.data_size = finfo.st_size;      
    sched_args2.num_map_threads = NUM_THREADS;
    sched_args2.num_reduce_threads = 1;
    sched_args2.num_merge_threads = NUM_THREADS;
    sched_args2.num_procs = NUM_THREADS;

    CHECK_ERROR(map_reduce_scheduler(&sched_args2) < 0);
    
    // Unmap input-prep.txt
    CHECK_ERROR(munmap(fdata, finfo.st_size + 1) < 0);
    CHECK_ERROR(close(fd) < 0);
    
    // Get execution time for phase 2 
    gettimeofday(&phase2_end, NULL);
    phase2_time = phase2_end.tv_sec - phase2_start.tv_sec + (phase2_end.tv_usec - phase2_start.tv_usec) / 1.e6;
    print_msg(1,"*********Phase 2 done: %g seconds******** \n\n", phase2_time);
   
      
    // Print RF matrix
    gettimeofday(&print_start, NULL);
    if (output == 1) {
      //sprintf(buffer, "mrsrf-output/mrsrf-%dtaxa-%dx%dtrees-node%d.txt", TAXA,NUM_ROW_TREES, NUM_COL_TREES, NODE_ID);
      sprintf(buffer, "mrsrf-mpi-output");
      if (stat(buffer, &st) !=0) {
        print_msg(1, "Directory %s does not exist.  MrsRF-MPI was supposed to create it.\n", buffer);
        print_msg(1, "RF matrix will not be printed!\n");
      }
      else {
        sprintf(buffer, "mrsrf-mpi-output/node%d.txt", NODE_ID);
        fout = fopen (buffer, "w");
       
        print_msg(1,"Printing RF Matrix:\n");
        int myval = 0;
        for (j = 0; j < TREE_BLOCK_SIZE; j++) {
          for (k = 0; k < NUM_COL_TREES; k++) {
            myval = (TAXA-3) - rf_matrix[j][k];
            fprintf(fout,"%d ", myval);
          }
          fprintf(fout, "\n");
        }
        /* Print new line at end of file like HashRF.  
           That way there is no difference between the output
			     of Mrs. RF and HashRF.
        */
        fprintf(fout, "\n");  
        fclose(fout);
      }
      gettimeofday(&print_end, NULL);
      print_time = print_end.tv_sec - print_start.tv_sec + (print_end.tv_usec - print_start.tv_usec) / 1.e6;
      print_msg(1,"*********Printing matrix done: %g seconds******** \n\n", print_time);
    }
  }

  // Deallocate rf_matrix 
  for (i = 0; i < TREE_BLOCK_SIZE; i++) {
		free(rf_matrix[i]);
  }
  free(rf_matrix);
    
    
  
    
  //fprintf(stderr, "\nRemoving temporary files \n");
  cleanup_dir();
    
  /* Get program execution time */
  gettimeofday(&prog_end, NULL);
  prog_time = prog_end.tv_sec - prog_start.tv_sec + (prog_end.tv_usec - prog_start.tv_usec) / 1.e6;
  print_msg(1, "********MrsRF-pq Completed on Node %d: %g seconds******** \n", NODE_ID, prog_time);
  print_msg(1, "Phase 1 time: %.2f%% of total time \n", phase1_time/prog_time * 100);
  print_msg(1, "Phase 2 time: %.2f%% of total time \n", phase2_time/prog_time * 100);
  if (output == 1) {
    print_msg(1, "Printing time: %.2f%% of total time \n\n", print_time/prog_time * 100);
  }

  return 0;
}
