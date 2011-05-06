#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void *safeMalloc(int s)
{
	void *retval = malloc(s);
	if(!retval)
	{
		fprintf(stderr, "Failed to allocate %d bytes of memory.\n", s);
		exit(1);
	}
	return retval;
}

int main(int argc, char **argv)
{
	char **newArgv;
	char *argCopy;
	int numArgs, i;

	if(argc < 2)
	{
		fprintf(stderr, "Usage: execute command\nCommand must be specified as a single parameter, e.g. execute \"/bin/ls -l\"\n");
		return 0;
	}

	/* strtok() modifies its input, so make a copy of argv[1] */
	argCopy = safeMalloc((strlen(argv[1]) + 1)*sizeof(char));
	strcpy(argCopy, argv[1]);
	
	/* Walk through argCopy and count the number of arguments */
	numArgs = 1;
	strtok(argCopy, " ");
	while(strtok(NULL, " "))
		numArgs++;

	/* Allocate newArgv[] */
	newArgv = safeMalloc((numArgs + 1)*sizeof(char *));
	/* Copy argv[1] again */
	strcpy(argCopy, argv[1]);

	/* Walk through argCopy again and populate newArgv[] */
	newArgv[0] = strtok(argCopy, " ");
	for(i = 1; i < numArgs; i++)
		newArgv[i] = strtok(NULL, " ");
	newArgv[i] = NULL;

	/* We're all set to call execve() now */
	execve(newArgv[0], newArgv, NULL);

	/* If we're still alive, something went wrong. Print an error message */
	perror("execute");
	return 1;
}

