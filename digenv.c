#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

int g_numPipes = 0;
int g_numProcs = 0;
int g_pipes[3][2] = {{0,0},{0,0},{0,0}};
pid_t g_procID[4] = {0, 0, 0, 0};

char * g_pager = NULL;

/* Initialize pipes */
void initPipes();

/* Sets pager */
void setPager();

/* Executes a command in a different process using fork */
void executeProcess(int in, int out, const char * command, int argc, char * argv[]);

/* Waits for processes to exit */
void waitForProcesses();

/* Kills all process children that are child to this process */
void killChildren();

/* Declaration of wait */
int wait(int * status);





int main (int argc, char * argv [])
{
	int pipeOffset = 0;
	g_numPipes = (argc > 1)? 3: 2;
	g_numProcs = g_numPipes + 1;
	
	/* Initialize pipes */
	initPipes();

	/* Set pager */
	setPager();
	
	
	/* Execute printenv */
	executeProcess(STDIN_FILENO, g_pipes[pipeOffset][1], "printenv", argc, argv);
	pipeOffset++;

	/* Check and execute grep */
	if(g_numPipes == 3)
	{
		executeProcess(g_pipes[pipeOffset - 1][0], g_pipes[pipeOffset][1], "grep", argc, argv);
		pipeOffset++;	
	}

	/* Execute sort */
	executeProcess(g_pipes[pipeOffset - 1][0], g_pipes[pipeOffset][1], "sort", argc, argv);
	pipeOffset++;

	/* Execute less or more */
 	executeProcess(g_pipes[pipeOffset - 1][0], STDOUT_FILENO, "less", argc, argv);

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
void executeProcess(int in, int out, const char * command, int argc, char * argv[])
{
	
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
		else
		{
			fprintf(stderr, "Child %d exited normally\n", child);
		}
	}
}
void killChildren()
{
	int i;
	for(i = 0; i < g_numProcs; i++)
	{
		if(g_procID[i] != 0) 
			kill(g_procID[i], SIGKILL);
	}
}
