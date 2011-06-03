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

void readCommand(char *buf, size_t size)
{
	int len;
	fgets(buf, size, stdin);
	len = strlen(buf);
	
	/* Trim the trailing newline, if present */
	if(buf[len - 1] == '\n')
		buf[len - 1] = '\0';
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
		readCommand(comm, 8192);
		c = parseCommandLine(comm);
		executeCommand(c);
		/* TODO: Implement backgrounding */
		waitForChildren(c);
		freeCommandList(c);
	}
	return EXIT_SUCCESS;
}
