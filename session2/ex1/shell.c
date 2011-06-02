#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "execute.h"

int main(int argc, char **argv)
{
	struct command *c;
	if(argc < 2)
	{
		printf("Usage: shell cmdline\n");
		return EXIT_SUCCESS;
	}
	c = parseCommandLine(argv[1]);
	executeCommand(c);
	freeCommandList(c);
	return EXIT_SUCCESS;
}
