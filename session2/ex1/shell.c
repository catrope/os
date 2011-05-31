#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define PUSH(p, head, tail) do { if(tail) { tail->next = p; tail = p; } else head = tail = p; } while(0)

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
	struct argument *firstArg; /* Pointer to the head of the argument list, or NULL if this is a redirection */ 
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
	c->firstArg = NULL;
	c->next = NULL;
	return c;
}

/**
 * Allocates a new string for the substring from start to end.
 * start and end must point in the same string.
 * Returned string includes start but not end, and is \0-terminated
 */
char *substring(char *start, char *end)
{
	char *retval = safeMalloc((end - start + 1)*sizeof(char));
	strncpy(retval, start, end - start);
	retval[end - start] = '\0';
	return retval;
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
	char *copy, *p, *q, *last, *fdString, *arg;
	int fd;
	
	int done = 0, inSingleQuotes = 0, inDoubleQuotes = 0, inRedir = 0, wasBackslash;

	copy = safeMalloc((strlen(commandLine) + 1)*sizeof(char));
	strcpy(copy, commandLine);
	cCurrent = newCommand();
	p = last = copy;
	while(!done)
	{
		switch(*p)
		{
			case ' ':
			case '|':
			case '\0':
				if(inSingleQuotes || inDoubleQuotes)
				{
					if(*p == '\0')
					{
						/* TODO: Error */
					}
					else
					{
						p++;
						break;
					}
				}
				
				if(!inRedir)
				{
					/* We're in a command, and just passed an argument. */
					if(p - last > 0)
					{
						/* Extract the argument */
						arg = substring(last, p);
						/* Create a struct argument for it */
						argS = safeMalloc(sizeof(struct argument));
						argS->s = arg;
						argS->next = NULL;
						
						/* Insert argS into the arg list, setting up the list if needed */
						PUSH(argS, cCurrent->firstArg, curTail);
					}
					
					if(*p == '|' || *p == '\0')
					{
						/* This is the end of the command */
						/* Insert the previous command into the command list */
						PUSH(cCurrent, cHead, cTail);
					}
					if(*p == '|')
					{
						/* Create a new command */
						cCurrent = newCommand();
						curTail = NULL;
					}
					if(*p == '\0')
						done = 1;
				}
				else if(p - last > 0)
				{
					/* We have found a file name for our redirection */
					rCurrent->filename = substring(last, p);
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
					PUSH(rCurrent, rHead, rTail);					
					inRedir = 0;
				}
				
				/* Skip the space or pipe character */
				last = ++p;
				break;
			case '>':
			case '<':
				/* Need to clean up previous arg here too. Bleeeehhhh. Will fix after reorg */
				if(inSingleQuotes || inDoubleQuotes)
				{
					p++;
					break;
				}
				if(inRedir)
				{
					/* TODO: Error. We're expecting a file */
				}
				
				/* This is a redirection */
				inRedir = 1;
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
				
				/* Set p past the <, > or >> . We expect a file after this */
				p += rCurrent->mode == OUTAPPEND ? 2 : 1;
				last = p;
				break;
			case '"':
			case '\'':
			case '\\':
				/* Backslash or quoted string */
				if((inSingleQuotes && *p == '"') || (inDoubleQuotes && *p == '\''))
				{
					/* Mismatching quote inside quotes. Pass it through */
					p++;
					break;
				}
				/* Toggle quote state */
				if(*p == '\'')
					inSingleQuotes = !inSingleQuotes;
				if(*p == '"')
					inDoubleQuotes = !inDoubleQuotes;
				wasBackslash = *p == '\\';
				
				/* Rewrite the quote or backslash out of the string by
				 * moving all following characters back
				 */
				for(q = p; *q; q++)
					*q = *(q + 1);
				
				/* If *p was a backslash, skip the next character */
				if(wasBackslash)
					p++;
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
