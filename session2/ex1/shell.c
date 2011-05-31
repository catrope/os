#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

enum redirMode { IN, OUT, OUTAPPEND };

struct redirection
{
	int fromfd; /* FD being redirected */
	char *filename; /* Filename to redirect to, or NULL if redirecting to an FD */
	int tofd; /* FD to redirect to. Only used if filename is NULL */
	enum redirMode mode; /* Mode to open the target in */
	struct redirection *next;
};

struct argument
{
	char *s;
	struct argument *next;
};

struct command
{
	struct argument *firstarg; /* Pointer to the head of the argument list, or NULL if this is a redirection */ 
	struct command *next;
};

void *safeMalloc(size_t s)
{
	void *r = malloc(s);
	if(!r)
	{
		fprintf(stderr, "Memory allocation failed. Tried to allocate %d bytes.\n", (int)s);
		exit(EXIT_FAILURE);
	}
	return r;
}

struct command *newCommand()
{
	struct command *c = safeMalloc(sizeof(struct command));
	c->firstarg = NULL;
	c->next = NULL;
	return c;
}

/**
 * Parse a command line, recognizing pipes and I/O redirection, and segmenting
 * argument lists.
 * @param commandLine Command line
 * @param redirs Set to a pointer to the head of a linked list of redirections
 * @return Pointer to the head of a linked list of commands
 */
struct command *parseCommandLine(const char *commandLine, struct redirection **redirs)
{
	struct command *cHead = NULL, *cTail = NULL, *cCurrent;
	struct redirection *rHead = NULL, *rTail = NULL, *rCurrent = NULL;
	struct argument *argS, *curTail = NULL;
	const char *p, *last, *fdString, *fileStart, *fileEnd;
	char *arg;
	int fd, done = 0;

	cCurrent = newCommand();
	p = last = commandLine;
	while(!done)
	{
		switch(*p)
		{
			case ' ':
			case '|':
			case '\0':
				/* We're in a command, and just passed an argument. */
				if(p - last > 0)
				{
					/* Extract the argument */
					arg = safeMalloc((p - last + 1)*sizeof(char));
					strncpy(arg, last, p - last);
					arg[p - last] = '\0';
					
					/* Create a struct argument for it */
					argS = safeMalloc(sizeof(struct argument));
					argS->s = arg;
					argS->next = NULL;
					
					/* Insert argS into the arg list, setting up the list if needed */
					if(curTail)
					{
						curTail->next = argS;
						curTail = argS;
					}
					else
						cCurrent->firstarg = curTail = argS;
					
				}
				
				if(*p == '|' || *p == '\0')
				{
					/* This is the end of the command */
					/* Insert the previous command into the command list */
					if(cTail)
					{
						cTail->next = cCurrent;
						cTail = cCurrent;
					}
					else
						/* Set up the list */
						cHead = cTail = cCurrent;
				}
				if(*p == '|')
				{
					/* Create a new command */
					cCurrent = newCommand();
					curTail = NULL;
				}
				if(*p == '\0')
					done = 1;
				
				/* Skip the space or pipe character */
				last = ++p;
				break;
			case '>':
			case '<':
				/* This is a redirection */
				/* Create a redirection object */
				rCurrent = safeMalloc(sizeof(struct redirection));
				rCurrent->next = NULL;
				
				/* Determine the redirection mode */
				if(*p == '<')
					rCurrent->mode = IN;
				else if(p[1] == '>')
					rCurrent->mode = OUTAPPEND;
				else
					rCurrent->mode = OUT;
				
				/* Look back for an FD number */
				fdString = p - 1;
				fd = 0;
				while(fdString >= last && isdigit(*fdString))
				{
					fd = 10*fd + *fdString - '0';
					fdString --;
				}
				/* Set the FD, using the found FD if found, or the default */
				rCurrent->fromfd = fd > 0 ? fd : (rCurrent->mode == IN ? 0 : 1);
				
				/* Look for a file */
				/* Set file past the <, > or >> */
				fileStart = p + (rCurrent->mode == OUTAPPEND ? 2 : 1);
				/* Skip spaces */
				while(*fileStart == ' ')
					fileStart++;
				/* Skip ahead to the next space    FIXME: or other special character */
				/* FIXME: Must be able to quote/escape spaces here */
				/* FIXME: May go out of bounds */
				fileEnd = fileStart;
				while(*fileEnd != ' ')
					fileEnd++;
				rCurrent->filename = safeMalloc((fileEnd - fileStart + 1)*sizeof(char));
				strncpy(rCurrent->filename, fileStart, fileEnd - fileStart);
				rCurrent->filename[fileEnd - fileStart] = '\0';
				
				/* Check if file is an FD (of the form &123) */
				if(rCurrent->filename[0] == '&' && isdigit(rCurrent->filename[1]))
				{
					rCurrent->tofd = atoi(&rCurrent->filename[1]);
					if(rCurrent->tofd > 0 || rCurrent->filename[1] == '0')
					{
						/* We have a good FD */
						free(rCurrent->filename);
						rCurrent->filename = NULL;
					}
					else
					{
						/* TODO: error */
					}
				}
				
				/* Add the redirection to the list */
				if(rTail)
				{
					rTail->next = rCurrent;
					rTail = rCurrent;
				}
				else
					/* Set up the list */
					rHead = rTail = rCurrent;
				
				/* Skip over what we parsed */
				p = last = fileEnd;
				break;
			case '"':
			case '\'':
				/* Quoted string */
				break;
			case '\\':
				/* Backslash, skip next character and rewrite the backslash out of the string */
				break;
			default:
				p++;
		}
	}
	
	*redirs = rHead;
	return cHead;
}

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
	return 0;
}
