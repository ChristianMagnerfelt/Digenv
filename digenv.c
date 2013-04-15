/*
 * NAME:
 *	digenv	-	view environment variables in alphabetical order using the default pager
 * 
 * SYNTAX:
 *	digenv [PATTERN]
 *
 * DESCRIPTION:
 *	Digenv sorts all environment variables in alphabetical order and displays them in
 *	the default pager (as specified by PAGER in the env. vars.), 'less' if no default pager
 *	is specified and 'more' if 'less' is not installed. 
 *
 *	The environment variables can also be filtered by using a regex pattern as one would
 *	when running grep.
 *
 * AUTHOR:
 *	Daniel Molin <dmol@kth.se>
 *	Christian Magnerfelt <christian.magnerfelt@gmail.com>
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

int g_numPipes = 0;							/* Number of pipes used in communication between processes */
int g_numProcs = 0;							/* Number of processes used in the pipeline */
int g_pipes[3][2] = {{0,0},{0,0},{0,0}};	/* IN, OUT file descriptors for each process */
pid_t g_procID[4] = {0, 0, 0, 0};			/* Process ID of each child process */

char * g_pager = NULL;						/* The pager used at the end of the pipeline */

/* Initialize pipes */
void initPipes();

/* Sets pager */
void setPager();

/* Executes a program in a different process using fork */
pid_t executeProcess(int in, int out, char * program, char * argv[]);

/* Waits for processes to exit */
void waitForProcesses();

/* Kills all process children that are child to this process */
void killChildren();

void closeFileDescriptors();

/* Declaration of wait */
int wait(int * status);

/* Handler for interrupt signal */
void sigIntHandler(int signum);

int main (int argc, char * argv [])
{
	/* Setup interrupt signal handler */
	signal(SIGINT, sigIntHandler);

	/* In case of command line arguments, add an extra process and pipe for grep */
	g_numPipes = (argc > 1)? 3: 2;	
	g_numProcs = g_numPipes + 1;
	
	/* Adjust offset if we are using 3 or 4 processes */
	int offset = (g_numProcs == 4)? 1: 0;	

	/* Initialize pipes */
	initPipes();

	/* Set pager */
	setPager();
	
	/* Execute printenv */
	g_procID[0] = executeProcess(STDIN_FILENO, g_pipes[0][1], "printenv", argv);

	/* Check and execute grep */
	if(g_numPipes == 3)
	{
		g_procID[1] = executeProcess(g_pipes[0][0], g_pipes[offset][1], "grep", argv);
	}

	/* Execute sort */
	g_procID[offset + 1] = executeProcess(g_pipes[offset][0], g_pipes[offset + 1][1], "sort", argv);

	/* Execute less or more */
 	g_procID[offset + 2] = executeProcess(g_pipes[offset + 1][0], STDOUT_FILENO, "less", argv);

	/* Close all file descriptors*/
	closeFileDescriptors();

	/* Wait for all processes to finish */
	waitForProcesses();
	return 0;
}
void initPipes()
{
	int i;
	int rc;
	for(i = 0; i < g_numPipes; i++)
	{
		rc = pipe(g_pipes[i]);
		/* exit if pipe fails */
		if(rc != 0)
		{
			fprintf(stderr, "Pipe failed : %d\n", errno);
			exit(1);
		}
	}
}
void setPager()
{
	g_pager = getenv("PAGER");
	if(g_pager == NULL)
	{
		g_pager = "less";
	}
	fprintf(stderr, "Pager set to '%s'\n", g_pager);
}
/*
 *	Executes a 'program' in a separate process using 'in' and 'out' as inter process communication.
 *		
 *	executeProcess start by duplicating 'in' and 'out' parameters to stdin and stdout in order to communicate with
 *	other processes in the pipeline. When file descriptors have been duplicated then remaining file descriptors are closed 
 *	in order to avoid deadlocks. Finally the 'program' is executed using execvp. If the program runs successfully this function will
 *	never return.
 */
pid_t executeProcess(int in, int out, char * program, char * argv[])
{
	pid_t id = fork();
	if(id == 0)
	{
		/* Dubplicate file descriptors in and out to stdin and stdout respectively */
		{
			int rc;
			rc = dup2(in, STDIN_FILENO);
			if(rc == -1 )
			{
				fprintf(stderr, "dup2 failed %d\n", errno);
				exit(1);
			}
			rc = dup2(out, STDOUT_FILENO);
			if(rc == -1 )
			{
				fprintf(stderr, "dup2 failed %d\n", errno);
				exit(1);
			}
		}
		/* Close all file descriptors*/
		closeFileDescriptors();

		/* Execute program */
		if(!strcmp(program, "grep"))
		{
			execvp(program, argv);
			fprintf(stderr, "Execvp %s failed %d\n", program, errno);
			exit(1);
		}
		else if(!strcmp(program, g_pager))
		{
			{
				char * ptr [2] = {program, (char*) NULL};
				execvp(program, ptr);
			}
			/* Default to more */
			{
				char * ptr [2] = {"more", (char*) NULL};
				execvp("more", ptr);
			}
			fprintf(stderr, "Execvp %s and more failed %d\n", program, errno);
			exit(1);		
		}
		else
		{
			char * ptr [2] = {program, (char*) NULL};
			execvp(program, ptr);
			fprintf(stderr, "Execvp %s failed %d\n", program, errno);
			exit(1);
		}
		
	}
	else if (id == -1)
	{
		fprintf(stderr, "fork() failed %d\n", errno);
		exit(1);
	}
	return id;
}
void waitForProcesses()
{
	int i;
	int status;
	pid_t child;
	for(i = 0; i < g_numProcs; i++)
	{
		/* Wait for child to finish */
		child = wait(&status);
		if(!WIFEXITED(status))
		{
			fprintf(stderr, "Child %d exited abnormally with status %d \n", child, status);
			/* Kill remaining, if any, processes */
			killChildren();
			exit(1);
		}
	}
}
void killChildren()
{
	int i;
	for(i = 0; i < g_numProcs; i++)
	{
		if(g_procID[i] != 0) 
		{
			int rc = kill(g_procID[i], SIGKILL);
			if(rc == -1)
			{
				fprintf(stderr, "kill of %d failed in killChildren() %d\n", g_procID[i], errno);
			}
		}
	}
}
void closeFileDescriptors()
{
	int i;
	for(i = 0; i < g_numPipes; i++)
	{
		int j;
		for(j = 0; j < 2; j++)
		{
			int rc = close(g_pipes[i][j]);
			if(rc == -1)
			{
				fprintf(stderr, "failed to close pipe g_pipes[%d][%d] == %d (errno: %d)\n", i, j, g_pipes[i][j], errno);
			}
		}
	}
}
void sigIntHandler(int signum)
{
	fprintf(stderr, "Process was interrupted: %d, abort\n", signum);
	killChildren();
	exit(1);	
}
