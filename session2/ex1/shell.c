#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "command.h"
#include "execute.h"

struct command *currentCommand;

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

void child(int sig, siginfo_t *info, void *context)
{
	int status;
	waitpid(info->si_pid, &status, 0);
}

void setupSignalHandlers()
{
	struct sigaction sigchld;
	sigchld.sa_sigaction = child;
	sigemptyset(&sigchld.sa_mask);
	sigchld.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction(SIGCHLD, &sigchld, NULL);	
}

int main(int argc, char **argv)
{
	struct command *c;
	char comm[8192];
	
	setupSignalHandlers();
	
	while(1)
	{
		showPrompt();
		if(!readCommand(comm, 8192))
		{
			putchar('\n');
			break;
		}
		
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
		waitForChildren(c);
		freeCommandList(c);
	}
	return EXIT_SUCCESS;
}
