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

void doCD(struct command *c)
{
	char *dir;
	dir = c->firstArg->next && c->firstArg->next->s ? c->firstArg->next->s : getenv("HOME");
	if(chdir(dir) < 0)
		perror(dir);
	
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
		{
			putchar('\n');
			break;
		}
		
		if(strlen(comm) == 0)
			continue;
		c = parseCommandLine(comm);
		
		/* Special commands */
		if(c->firstArg && c->firstArg->s)
		{
			/* Special commands */
			if(!strcmp(c->firstArg->s, "exit"))
				break;
			if(!strcmp(c->firstArg->s, "cd"))
			{
				doCD(c);
				continue;
			}
		}
		
		
		executeCommand(c);
		/* TODO: Implement backgrounding */
		waitForChildren(c);
		freeCommandList(c);
	}
	return EXIT_SUCCESS;
}
