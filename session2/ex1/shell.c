#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "command.h"
#include "execute.h"

void showPrompt()
{
	printf("%c ", geteuid() ? '$': '#');
	fflush(stdout);
}

int readCommand(char *buf, size_t size)
{
	int len, retval;
	retval = fgets(buf, size, stdin) ? 1 : 0;
	len = strlen(buf);
	
	/* Trim the trailing newline, if present */
	if(buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	return retval;
}

/* TODO: sigchild handler for backgrounding */
/* TODO: Ctrl+C handling */

int main(int argc, char **argv)
{
	struct command *c;
	char comm[8192];
	
	while(1)
	{
		showPrompt();
		if(!readCommand(comm, 8192))
			break;
		c = parseCommandLine(comm);
		executeCommand(c);
		/* TODO: Implement backgrounding */
		waitForChildren(c);
		freeCommandList(c);
	}
	puts("\nBye");
	return EXIT_SUCCESS;
}
