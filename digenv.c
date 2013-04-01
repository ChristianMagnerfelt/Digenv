#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int g_numPipes;

int g_pipes[3][2];

/* Executes a command in a different process using fork */
void executeProcess(int in, int out, const char * command, int argc, char * argv[]);

int main (int argc, char * argv [])
{
	int pipeOffset = 0;
	g_numPipes = (argc > 1)? 3: 2;
	{
		int i;
		for(i = 0; i < g_numPipes; i++)
		{
			pipe(g_pipes[i]);
		}
	}

	/* Set pager */
	
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

	return 0;
}

void executeProcess(int in, int out, const char * command, int argc, char * argv[])
{
	
}
