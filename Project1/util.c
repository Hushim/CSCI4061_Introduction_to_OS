/************************
 * util.c
 *
 * utility functions
 *
 ************************/

#include "util.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/***************
 * These functions are just some handy file functions.
 * We have not yet covered opening and reading from files in C,
 * so we're saving you the pain of dealing with it, for now.
 *******/
FILE* file_open(char* filename) {
	FILE* fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "make4061: %s: No such file or directory.\n", filename);
		exit(1);
	}
	return fp;
}

char* file_getline(char* buffer, FILE* fp) {
	buffer = fgets(buffer, 1024, fp);
	return buffer;
}

//Return -1 if file does not exist
int is_file_exist(char * lpszFileName)
{
	return access(lpszFileName, F_OK); 
}

int get_file_modification_time(char * lpszFileName)
{
	if(is_file_exist(lpszFileName) != -1)
	{
		struct stat buf;
		int nStat = stat(lpszFileName, &buf);
		return buf.st_mtime;
	}
	
	return -1;
}

// Compares the timestamp of two files
int compare_modification_time(char * fileName1, char * fileName2)
{	
	int nTime1 = get_file_modification_time(fileName1);
	int nTime2 = get_file_modification_time(fileName2);

	//printf("%s - %d  :  %s - %d\n", fileName1, nTime1, fileName2, nTime2);

	if(nTime1 == -1 || nTime2 == -1)
	{
		return -1;
	}

	if(nTime1 == nTime2)
	{
		return 0;
	}
	else if(nTime1 > nTime2)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

// makeargv
/* Taken from Unix Systems Programming, Robbins & Robbins, p37 */
int makeargv(const char *s, const char *delimiters, char ***argvp) {
   int error;
   int i;
   int numtokens;
   const char *snew;
   char *t;

   if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) {
      errno = EINVAL;
      return -1;
   }
   *argvp = NULL;
   snew = s + strspn(s, delimiters);
   if ((t = malloc(strlen(snew) + 1)) == NULL)
      return -1;
   strcpy(t,snew);
   numtokens = 0;
   if (strtok(t, delimiters) != NULL)
      for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++) ;

   if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL) {
      error = errno;
      free(t);
      errno = error;
      return -1;
   }

   if (numtokens == 0)
      free(t);
   else {
      strcpy(t,snew);
      **argvp = strtok(t,delimiters);
      for (i=1; i<numtokens; i++)
         *((*argvp) +i) = strtok(NULL,delimiters);
   }

   *((*argvp) + numtokens) = NULL;
   return numtokens;
}

void freemakeargv(char **argv) {
   if (argv == NULL)
      return;
   if (*argv != NULL)
      free(*argv);
   free(argv);
}

// Used to find the index of target with targetName = lpszTargetName from the list of targets "t"
int find_target(char * lpszTargetName, target_t * const t, int const nTargetCount)
{
	int i=0;
	for(i=0;i<nTargetCount;i++)
	{
		if(strcmp(lpszTargetName, t[i].szTarget) == 0)
		{
			return i;
		}
	}

	return -1;
}

/* Parsing function to parse the Makefile with name = lpszFileName and return the number of targets.
   "t" will point to the first target. Revise "Arrays and Pointers" section from Recitation 2.
*/
int parse(char * lpszFileName, target_t * const t)
{
	char szLine[1024];
	char * lpszLine = NULL;
	char * lpszTargetName = NULL;
	char * lpszDependency = NULL;
	char **prog_args = NULL;
	FILE * fp = file_open(lpszFileName);
	int nTargetCount = 0;
	int nLine = 0;
	target_t * pTarget = t;
	int nPreviousTarget = 0;

	if(fp == NULL)
	{
		return -1;
	}

	while(file_getline(szLine, fp) != NULL) 
	{
		nLine++;
		// this loop will go through the given file, one line at a time
		// this is where you need to do the work of interpreting
		// each line of the file to be able to deal with it later
		lpszLine = strtok(szLine, "\n"); //Remove newline character at end if there is one

		if(lpszLine == NULL || *lpszLine == '#') //skip if blank or comment
		{
			continue;
		}

		//Remove leading whitespace
		while(*lpszLine == ' ')
		{
			lpszLine++;
		}
		
		//skip if whitespace-only
		if(strlen(lpszLine) <= 0) 
		{
			continue;
		}

		//Multi target is not allowed.
		if(*lpszLine == '\t') //Commmand
		{
			lpszLine++;

			if(strlen(pTarget->szTarget) == 0)
			{
				fprintf(stderr, "%s: line:%d *** specifying multiple commands is not allowed.  Stop.\n", lpszFileName, nLine);
				return -1;
			}

			strcpy(pTarget->szCommand, lpszLine);
			if (makeargv(pTarget->szCommand, " ", &prog_args) == -1) 
			{
				perror("Error parsing command line");
				exit(EXIT_FAILURE);
			}
			pTarget->prog_args = prog_args;
			nPreviousTarget = 0;
			pTarget++;
		}
		else	//Target
		{
			//check : exist Syntax check 
			if(strchr(lpszLine, ':') == NULL)
			{
				fprintf(stderr, "%s: line:%d *** missing separator.  Stop.\n", lpszFileName, nLine);
				return -1;
			}

			//Previous target don't have a command
			if(nPreviousTarget == 1)
			{
				pTarget++;
			}

			//Not currently inside a target, look for a new one
			lpszTargetName = strtok(lpszLine, ":");

			if(lpszTargetName != NULL && strlen(lpszTargetName) > 0)
			{
				strcpy(pTarget->szTarget, lpszTargetName);
				lpszDependency = strtok(NULL, " ");

				while (lpszDependency != NULL)
				{
					strcpy(pTarget->szDependencies[pTarget->nDependencyCount], lpszDependency);
					pTarget->nDependencyCount++;
					lpszDependency = strtok(NULL, " ");
				}

				nTargetCount++;
				nPreviousTarget = 1;
			}
			else //error
			{
				fprintf(stderr, "%s: line:%d *** missing separator.  Stop.\n", lpszFileName, nLine);
				return -1;
			}
		}
	}

	fclose(fp);

	return nTargetCount;
}

// Function to print the data structure populated by parse() function.
// Use prog_args as arguments for execvp()
void show_targets(target_t * const t, int const nTargetCount)
{
	int i=0;
	int j=0;
	int k=0;
	for(i=0;i<nTargetCount;i++)
	{
		k = 0;
		printf("%d. Target: %s  Status: %d\nCommand: %s\nDependency: ", i, t[i].szTarget, t[i].nStatus, t[i].szCommand); 

		for(j=0;j<t[i].nDependencyCount;j++)
		{
			printf("%s ", t[i].szDependencies[j]); 
		}
		printf("\nDecomposition of command:\n\t");
		while (t[i].prog_args[k] != NULL) {
			printf("%d. %s\n\t", k, t[i].prog_args[k]);
			k++;
		}
		printf("\n\n");
	}
}

//check duplicated target
int check_duplicated_targets(target_t *const targets, int targets_len)
{
	int i ,j = 0;
	for(i = 0; i < targets_len; i++)
	{
		char *temp_target_name = targets[i].szTarget; 
		for(j = i+1; j < targets_len; j++)
		{
			//duplicated targets
			if(!strcmp(temp_target_name, targets[j].szTarget))
			{
				fprintf(stderr, "target %s is duplicated\n", temp_target_name);
				return -1;
			}
		}
	}
	//printf("No duplicated targets\n");
	return 0;
}

//construct targets binary relation matrix
int construct_targets_and_source_files_matrix(target_t *const targets, int targets_len, int **matrix, int **source_files_matrix)
{
	int index = 0 ;
	for(index = 0; index < targets_len; index ++)
	{
		int denpendency_index = targets[index].nDependencyCount;
		int i = 0;
		for(i = 0 ; i < denpendency_index; i++)
		{
			//printf(stderr,"%s\n", targets[index].szDependencies[i]);
			int corresp_index_in_target_lists = find_target(targets[index].szDependencies[i], targets, targets_len);

			//check if dependency exist in target lists;
			if( corresp_index_in_target_lists == -1 )
			{
				source_files_matrix[index][i] = 1;// maket the file may not be a targets or missing. 
			}
			else
			{
				matrix[index][corresp_index_in_target_lists] = 1;	
			}
			
		}
	}
	return 0;
}

//dynamically create a 2D matrix
int create_matrix(int ***matrix, int row, int col)
{
	*matrix = (int **)calloc(row, sizeof(int *));
	//check err in case  alloc fails
	if(*matrix == NULL)
	{
		fprintf(stderr, "alloc fialed\n");
		return -1;
	}

	int i = 0;
	for(i = 0; i < row; i++)
	{
		(*matrix)[i] = (int *)calloc(col, sizeof(int));

		//check err in case alloc fials
		if((*matrix)[i] == NULL)
		{
			fprintf(stderr, "alloc fialed\n");
			return -1;
		}
	}
	return 0;
}

//free a 2D matrix
int free_matrix(int ***matrix, int row)
{
	int i = 0;
	for(i = 0 ; i < row; i++)
	{
		free((*matrix)[i]);
		//printf("free memory: matrx[%d]\n", i);
	}
	free(*matrix);
	//printf("free matrix \n");

	return 0;
}

int initial_matrix(int **matrix, int row, int col)
{
	//memset(matrix, 0, sizeof(int)*targets_len*targets_len);
	//printf("Initial matrix\n");
	int i ,j = 0;
	for(i = 0 ; i < row; i++)
	{
		for(int j = 0 ; j < col; j++)
		{
			matrix[i][j] = 0;
		}
	}
	return 0;
}

int display_matrix(int **matrix, int row, int col)
{
	printf("display_matrixs\n");
	int i, j = 0;
	for(i = 0; i < row; i++)
	{
		for(j = 0; j < col; j++)
		{
			printf("%d ", matrix[i][j]);
		}
		printf("\n");
	}
	return 0;
}

//check dependency by binary relation targets matrix and missing files matrix
int check_denpendency(target_t *const targets, 
					  int **const targets_matrix, 
					  int **const source_files_matrix, 
					  int targets_matrix_row_or_col,
					  int source_files_matrix_col)
{
	int num_of_missing_tagets = 0;
	int i, j = 0;
	for(i = 0 ; i < targets_matrix_row_or_col; i++)
	{
		for(j = 0 ; j < targets_matrix_row_or_col; j++){
			if(targets_matrix[i][j]){
				if(find_target(targets[j].szTarget, targets, targets_matrix_row_or_col) == -1)
				{
					num_of_missing_tagets++;
					fprintf(stderr, " Failed to find target %s\n", targets[j].szTarget);
					return num_of_missing_tagets;
				}
			}
		}

		for(j= 0 ; j < source_files_matrix_col; j++)
		{
			if(source_files_matrix[i][j])
			{
				if(is_file_exist(targets[i].szDependencies[j]) == -1)
				{
					num_of_missing_tagets++;
					fprintf(stderr, "*** No rule to make target `%s', needed by `%s',\n",targets[i].szDependencies[j], targets[i].szTarget);
					return num_of_missing_tagets;
				}
			}
		}
	}
	return num_of_missing_tagets;
}

//might be used for future
//check if the target is root target, otherwise return the level from root target
// int find_root_target(int target_index, int **targets_matrix, int targets_matrix_row_or_col,int *level)
// {
// 	int no_deoendency = 0;
// 	int i;
// 	while(!no_deoendency){
// 		for(i = 0; i < targets_matrix_row_or_col; i++)
// 		{
// 			if(targets_matrix[i][target_index])
// 			{
// 				target_index = i;
// 				(*level)++;
// 				break;
// 			}
// 			no_deoendency = 1;
// 		}
// 	}
// 	//printf("!!!!! %d !!!!\n", target_index);
// 	return target_index;
// }

int check_timestamp(target_t *targets,
					int target_index,
					int **const targets_matrix,
					int targets_matrix_row_or_col,
					int **source_files_matrix,
					int source_files_matrix_col)
{
	int col =0;
	int count = 0;
	if(is_file_exist(targets[target_index].szTarget) == -1)
	{
		//printf("the file %s is not available\n", targets[target_index].szTarget);
		targets[target_index].nStatus = READY;
		count++;	
	}
	for( col = 0; col < targets_matrix_row_or_col; col++)
	{
		if(targets_matrix[target_index][col])
		{
			{
				//printf("check the the terget: %s 's denpendecy target: %s\n", targets[i].szTarget, targets[j].szTarget);
				int tmp = compare_modification_time(targets[target_index].szTarget, targets[col].szTarget);
				if(tmp != 1 && tmp != 0 && tmp != -1)
				{
					//printf("return number: %d, The target: %s is less recnet than target: %s \n",tmp, targets[target_index].szTarget, targets[col].szTarget);
					targets[target_index].nStatus = READY;
					count++;
					break;
				}
				else
				{
					int tmp_count = check_timestamp(targets,
													col,
								   					targets_matrix,
					               					targets_matrix_row_or_col,
					               					source_files_matrix,
					               					source_files_matrix_col);
					if(tmp_count)
					{
						targets[target_index].nStatus = READY;
						count += tmp_count;
					}
				}
			}
		}
	}
	if(targets[target_index].nStatus != READY)
	{
		int j = 0;
		for(j = 0; j < source_files_matrix_col; j++)
		{
			if(source_files_matrix[target_index][j])
			{
				//printf("check target: %s's source file %s\n",targets[target_index].szTarget, targets[target_index].szDependencies[j]);
				int tmp = compare_modification_time(targets[target_index].szTarget, targets[target_index].szDependencies[j]);
				if(tmp != 1 && tmp != 0)
				{	
					//printf("return number: %d, The target: %s is less recnet than target: %s \n", tmp, targets[target_index].szTarget, targets[target_index].szDependencies[j]);
					targets[target_index].nStatus = READY;
					count++;
					break;
				}
			}
		}
	}
	return count;
}

// excute commands for each target recusively with or without -n and -B flag
int excute(target_t *const targets, int target_index, int **targets_matrix, int targets_matrix_row_or_col, int recompile_flag, int no_excute_flag)
{
	int col = 0;
	for(col = 0 ; col < targets_matrix_row_or_col; col++)
	{
		if(targets_matrix[target_index][col])
		{
			if(excute(targets, col, targets_matrix, targets_matrix_row_or_col, recompile_flag, no_excute_flag) != 1)
			{
				printf("Failed to print command at target[%d]\n", col);
			}
		}
	}

	if(!no_excute_flag)
	{
		//without -n flag
		int pid = fork();
		int status;

		if(pid > 0)
		{	//parent

			if(wait(&pid) < 0){
				perror("Child exited with error!\n");
				exit(-1);
			}	
		}
		else if(pid == 0)
		{	//child
			if(targets[target_index].nStatus == READY || recompile_flag)
			{
				printf("%s\n", targets[target_index].szCommand);
				targets[target_index].nStatus == FINISHED;
				if(execvp(targets[target_index].prog_args[0],targets[target_index].prog_args) != 0);
				{
					perror("exevp error!\n");
					exit(-1);
				}
			}
			else{
				exit(1);
			}	
		}
		else
		{
			perror("Fork failed!\n");
			exit(-1);
		}	
	}
	else
	{	//with -n flag
		if(targets[target_index].nStatus == READY || recompile_flag)
		{
			targets[target_index].nStatus == FINISHED;
			printf("%s\n", targets[target_index].szCommand);
		}
	}

	
	return 1;
}