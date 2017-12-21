/* CSci4061 F2016 Assignment 1
* login: cselabs login name (login used to submit)
* date: 10/5/16
* name: Shimao Hu, Hansi Ji, Montana Gau (for partner(s))
* id: huxxx952, jixxx217, gauxx026 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <unistd.h>

#include "util.h"

void show_error_message(char * lpszFileName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", lpszFileName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
	fprintf(stderr, "-B\t\tDon't check files timestamps.\n");
	exit(0);
}

int main(int argc, char **argv) 
{
	target_t targets[MAX_NODES]; //List of all the targets. Check structure target_t in util.h to understand what each target will contain.
	int nTargetCount = 0;

	// Declarations for getopt
	extern int optind;
	extern char * optarg;
	int ch;
	char * format = "f:hnB";
	
	// Variables you'll want to use
	char szMakefile[64] = "Makefile";
	char szTarget[64] = "";
	int i=0;

	//flag
	int re_compile = 0;
	int no_excute = 0;


	//targets_matrix
	int **targets_matrix = NULL;
	int **source_files_matrix = NULL;
	
	//init Targets 
	for(i=0;i<MAX_NODES;i++)
	{
		targets[i].pid=0 ;
		targets[i].nDependencyCount = 0;
		strcpy(targets[i].szTarget, "");
		strcpy(targets[i].szCommand, "");
		targets[i].nStatus = FINISHED;
	}

	while((ch = getopt(argc, argv, format)) != -1) 
	{
		switch(ch) 
		{
			case 'f':
				strcpy(szMakefile, strdup(optarg));
				break;
			case 'n':
				//Set flag which can be used later to handle this case.
				no_excute =1;
				break;
			case 'B':
				//Set flag which can be used later to handle this case.
				re_compile = 1;
				break;
			case 'h':
			default:
				show_error_message(argv[0]);
				exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if(argc > 1)
	{
		show_error_message(argv[0]);
		return EXIT_FAILURE;
	}

	/* Parse graph file or die */
	if((nTargetCount = parse(szMakefile, targets)) == -1) 
	{
		return EXIT_FAILURE;
	}

	//just test to show the demision of the matrix

	//Setting Targetname
	//if target is not set, set it to default (first target from makefile)
	if(argc == 1)
	{
		strcpy(szTarget, argv[0]);
	}
	else
	{
		strcpy(szTarget, targets[0].szTarget);
	}

	show_targets(targets, nTargetCount);

	//Now, the file has been parsed and the targets have been named. You'll now want to check all dependencies (whether they are available targets or files) and then execute the target that was specified on the command line, along with their dependencies, etc. Else if no target is mentioned then build the first target found in Makefile.
	
	 if(check_duplicated_targets(targets, nTargetCount) == -1)
	{
		return EXIT_FAILURE;
	}

	//dynamically create a matrix to show binary relation
	 if(create_matrix(&targets_matrix, nTargetCount, nTargetCount) == -1)
	 {
	 	fprintf(stderr, "Fialed to create dynamic targets matrix\n");
	 	exit(EXIT_FAILURE);
	 }

	//dynamically create a array to store missing files' names

	 if(create_matrix(&source_files_matrix, nTargetCount, MAX_DEPENDENCIES) == -1)
	 {
	 	fprintf(stderr, "Fialed to create dynamic missing files matrix\n");
	 	exit(EXIT_FAILURE);
	 }

	//initilize a targets matrix to 0
	if(initial_matrix(targets_matrix, nTargetCount, nTargetCount) != 0)
	{
		fprintf(stderr, "Failed to initialize matrix\n");
	}

	//initilize missing files matrix to 0
	if(initial_matrix(source_files_matrix, nTargetCount, MAX_DEPENDENCIES) != 0)
	{
		fprintf(stderr, "Failed to initialize matrix\n");
	}
	
	//display_matrix(source_files_matrix, nTargetCount, MAX_DEPENDENCIES);
	
	//construct binary relation by matrix
	construct_targets_and_source_files_matrix(targets, nTargetCount, targets_matrix, source_files_matrix);
	//display establisehd targets_matrix
	//display_matrix(targets_matrix, nTargetCount, nTargetCount);
	//display estarbilished source_files_matrix
	//display_matrix(source_files_matrix, nTargetCount, MAX_DEPENDENCIES);


	if(check_denpendency(targets, targets_matrix,
						 source_files_matrix,
						 nTargetCount,
						 MAX_DEPENDENCIES) > 0)
	{
		fprintf(stderr, "Can not find required files");
		exit(EXIT_FAILURE);
	}

	//To find the first target index and prepare for excute
	int target_index =find_target(szTarget, targets, nTargetCount);

	if(target_index == -1)
	{
		fprintf(stderr, "*** No rule to make target %s\n", szTarget);
		exit(EXIT_FAILURE);
	}

	/**
	This part is to find the root target, which might be used for future
	*/
	//int level = 0; // currentTarget level from the roo
	//check if it is root target;
	// int root_index = find_root_target(target_index, targets_matrix, nTargetCount, &level);
	// if(root_index != target_index)
	// {
	// 	fprintf(stderr, "The %s is not root target, which is at level %d from root\n The root target is %s", szTarget, level, targets[root_index].szTarget);
	// }else
	// {
	// 	fprintf(stderr, "The %s is root target\n", szTarget);
	// }

	//check timestamps and chenged the corresponding parts
	if(check_timestamp(targets,
					   target_index,
					   targets_matrix,
					   nTargetCount,
					   source_files_matrix,
					   MAX_DEPENDENCIES) <= 0 && !re_compile)
	{
		// int i = 0;
		// for(i = 0 ; i < nTargetCount; i++)
		// {
		// 	printf("%d ", targets[i].nStatus);
		// }
		// printf("\n");
		printf("%s is up to date!\n",targets[target_index].szTarget);
		exit(EXIT_FAILURE);
	}

	//excute makefile or just print commands, which depends on re_compile flag or no_excute flag
	if(excute(targets, target_index, targets_matrix, nTargetCount, re_compile, no_excute) != 1)
	{
		fprintf(stderr, "Failed to excute");
		exit(EXIT_FAILURE);
	}

	//free two 2D matrix which is to buid binary relation between targets and between targets and dependency
	if(free_matrix(&source_files_matrix, nTargetCount) != 0)
	{
		fprintf(stderr, "Failed to free source_files_matrix\n" );
		exit(EXIT_FAILURE);
	}

	if(free_matrix(&targets_matrix, nTargetCount) != 0)
	{
		fprintf(stderr, "Failed to free targets matrix\n");
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
