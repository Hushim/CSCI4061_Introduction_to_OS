/********************
 * util.h
 *
 * You may put your utility function definitions here
 * also your structs, if you create any
 *********************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// the following ifdef/def pair prevents us from having problems if 
// we've included util.h in multiple places... it's a handy trick
#ifndef _UTIL_H_
#define _UTIL_H_

// Implementation dependent, change or don't use based on your requirement.
#define INELIGIBLE 0
#define READY 1
#define RUNNING 2
#define FINISHED 3

#define MAX_LENGTH 1024
#define MAX_CHILDREN 10
#define MAX_NODES 10
#define MAX_DEPENDENCIES 10

// This stuff is for easy file reading, and parsing the makefile and populating the data structure
FILE * file_open(char*);
char * file_getline(char*, FILE*);
int is_file_exist(char *);
int get_file_modification_time(char *);
int compare_modification_time(char *, char *);

//Build and run the project with given Makefile to see the contents.
typedef struct target{
	pid_t pid; //Pid of the process that is handling this target.
	char szTarget[64]; //Target name
	int nDependencyCount; //Number of dependencies
	char szDependencies[10][64]; //Names of all the dependencies
	char szCommand[256]; //Command that needs to be executed for this target
	char **prog_args; //Command decomposed into different parts. Use this as arguments for execvp() function.
	int nStatus; //Status of the target(Running, Finished etc. based on your implementation)
}target_t;

int find_target(char * lpszTargetName, target_t * const t, int const nTargetCount);
int parse(char * lpszFileName, target_t * const t);
void show_targets(target_t * const t, int const nTargetCount);
int makeargv(const char *s, const char *delimiters, char ***argvp);
void freemakeargv(char **argv);

int check_targets_len(target_t *const targets);
int check_duplicated_targets(target_t *const targets, int targets_len);
int create_matrix(int ***matrix, int row, int col);
int free_matrix(int ***matrix, int row);
int initial_matrix(int **matrix, int row, int col);
int construct_targets_and_source_files_matrix(target_t *const targets, int targets_len, int **matrix, int **ssource_files_matrix);
int display_matrix(int **matrix, int row, int col);
int check_denpendency(target_t *const targets, 
					  int **const targets_matrix, 
					  int **const source_files_matrix, 
					  int targets_matrix_row_or_col,
					  int source_files_matrix_col);
int check_timestamp(target_t *targets,
					int target_index,
					int **const targets_matrix,
					int targets_matrix_row_or_col,
					int **source_files_matrix,
					int source_files_matrix_col);
//int find_root_target(int target_index, int **targets_matrix, int targets_matrix_row_or_col,int *level);
int excute(target_t *const targets, int target_index, int **targets_matrix, int targets_matrix_row_or_col, int recompile_flag, int no_excute_flag);
#endif
