#include <stdio.h>
#include <stdlib.h>
#include "command.h"

int main(int argc, char **argv)
{
	struct command *c;
	struct redirection *r;
	if(argc < 2)
	{
		printf("Usage: shell cmdline\n");
		return EXIT_SUCCESS;
	}
	c = parseCommandLine(argv[1], &r);
	return EXIT_SUCCESS;
}
