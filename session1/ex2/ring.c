#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXNUMBER 50

void *safeMalloc(int s)
{
        void *retval = malloc(s);
        if(!retval)
        {
                printf("Failed to allocate %d bytes of memory.\n", s);
                exit(1);
        }
        return retval;
}

void child(int readFD, int writeFD)
{
	int number = 0, pid;
	pid = getpid();
	while(number <= MAXNUMBER)
	{
		read(readFD, &number, sizeof(int));
		if (number <= MAXNUMBER)
			printf("pid=%d: %d\n", pid, number);
		
		/* Send the incremented number to the next process, even if
		 * it's too high. This makes sure all children terminate. */
		number++;
		write(writeFD, &number, sizeof(int));
	}
}

int main(int argc, char **argv)
{
	int *pids, *pipes;
	int i, j, numProcs, pid, readFD, writeFD;

	if(argc < 2)
	{
		puts("Usage: ring numProcs\n");
		return 0;
	}

	numProcs = atoi(argv[1]);
	if(numProcs < 2)
	{
		puts("Error: number of processes must be at least 2.\n");
		return 1;
	}

	pids = safeMalloc(numProcs*sizeof(int));
	pipes = safeMalloc(2*numProcs*sizeof(int));

	/* Set up pipes */
	for(i = 0; i < numProcs; i++)
		pipe(&pipes[2*i]);
	
	/* Fork child processes */
	for(i = 0; i < numProcs; i++)
	{
		pid = fork();
		if(pid)
			/* Parent */
			pids[i] = pid;
		else
		{
			/* Child */
			/* Close the FDs we don't need */
			readFD = pipes[2*i];
			writeFD = i < numProcs - 1 ? pipes[2*i + 3] : pipes[1];
			for(j = 0; j < 2*numProcs; j++)
				if(pipes[j] != readFD && pipes[j] != writeFD)
					close(pipes[j]);
			/* Run the child code */
			child(readFD, writeFD);
			close(readFD);
			close(writeFD);
			exit(0);
		}
	}

	/* Write a zero to start the chain */
	j = 0;
	write(pipes[1], &j, sizeof(int));

	/* Close all pipes, we don't need them any more in the parent */
	for(j = 0; j < 2*numProcs; j++)
		close(pipes[j]);

	/* Wait for all children */
	for(i = 0; i < numProcs; i++)
		waitpid(pids[i], NULL, 0);

	return 0;
}

