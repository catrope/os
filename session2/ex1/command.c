#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "command.h"

#define PUSH(p, head, tail) do { if(tail) { (tail)->next = (p); (tail) = (p); } else (head) = (tail) = (p); } while(0)

void freeArgumentList(struct argument *head)
{
	if(head->next)
		freeArgumentList(head->next);
	if(head->s)
		free(head->s);
	free(head);
}

void freeRedirectionList(struct redirection *head)
{
	if(head->next)
		freeRedirectionList(head->next);
	if(head->filename)
		free(head->filename);
	free(head);
}

static void freeCommand_internal(struct command *head, int recursive)
{
	if(head->next && recursive)
		freeCommand_internal(head->next, recursive);
	if(head->firstArg)
		freeArgumentList(head->firstArg);
	if(head->redir)
		freeRedirectionList(head->redir);
	free(head);
}

void freeCommandList(struct command *head)
{
	freeCommand_internal(head, 1);
}

void freeCommand(struct command *cmd)
{
	freeCommand_internal(cmd, 0);
}

static struct command *newCommand()
{
	struct command *c = safeMalloc(sizeof(struct command));
	c->firstArg = NULL;
	c->redir = NULL;
	c->mode = 0;
	c->next = NULL;
	return c;
}

/**
 * Allocates a new string for the substring from start to end.
 * start and end must point in the same string.
 * Returned string includes start but not end, and is \0-terminated
 */
static char *substring(char *start, char *end)
{
	char *retval = safeMalloc((end - start + 1)*sizeof(char));
	strncpy(retval, start, end - start);
	retval[end - start] = '\0';
	return retval;
}

static void pushArg(char *start, char *end, struct argument **head, struct argument **tail)
{
	char *arg;
	struct argument *argS;
	arg = substring(start, end);
	argS = safeMalloc(sizeof(struct argument));
	argS->s = arg;
	argS->next = NULL;
	PUSH(argS, *head, *tail);
}

static void pushFile(char *start, char *end, struct redirection *current, struct redirection **head, struct redirection **tail)
{
	current->filename = substring(start, end);
	/* Check if file is an FD (of the form &123) */
	if(current->filename[0] == '&' && isdigit(current->filename[1]))
	{
		current->tofd = atoi(&current->filename[1]);
		if(current->tofd > 0 || current->filename[1] == '0')
		{
			/* We have a good FD */
			free(current->filename);
			current->filename = NULL;
		}
		/* We could throw an error if we didn't find a good FD, but that's not necessary.
		 * We'll fall through to interpreting the bad FD as a file name instead. */
	}
	
	/* Add the redirection to the list */
	PUSH(current, *head, *tail);
}

struct command *parseCommandLine(const char *commandLine)
{
	struct command *cHead = NULL, *cTail = NULL, *cCurrent, *chainStart = NULL;
	struct redirection *rTail = NULL, *rCurrent = NULL;
	struct argument *curTail = NULL;
	char *copy, *p, *q, *last, *fdString;
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
			case '&':
			case '\0':
				/* *p != '\0' check is needed so we don't try to increment p
				 * past the end. In the *p == '\0' case we have an unbalanced
				 * quote. We could throw an error in that case, but we'll just
				 * ignore it instead: this will automatically handle it gracefully
				 * by effectively pretending there is a closing quote at the end.
				 */
				if((inSingleQuotes || inDoubleQuotes) && *p != '\0')
				{
						p++;
						break;
				}
				
				if(!inRedir)
				{
					/* We're in a command, and just passed an argument. */
					if(p - last > 0)
						pushArg(last, p, &cCurrent->firstArg, &curTail);
					
					if(*p == '&')
					{
						/* Background the entire pipe chain */
						if(!chainStart && cHead)
						{
							chainStart = cHead;
							chainStart->mode |= BACKGROUND;
							while(chainStart->next)
							{
								chainStart->mode |= BACKGROUND;
								chainStart = chainStart->next;
							}
						}
						cCurrent->mode |= BACKGROUND;
					}
					if(*p == '|')
						cCurrent->mode |= PIPED;
					
					if(*p == '|' || *p == '&' || *p == '\0')
						/* This is the end of the command */
						/* Insert the previous command into the command list if non-empty */
						if(cCurrent->firstArg)
							PUSH(cCurrent, cHead, cTail);
					
					if(*p == '|' || *p == '&')
					{
						/* Create a new command */
						cCurrent = newCommand();
						curTail = NULL;
						rTail = NULL;
					}
					if(*p == '\0')
						done = 1;
				}
				else
				{
					if(*p == '&')
					{
						/* Signals an FD; handled by pushFile(), leave it alone */
						p++;
						break;
					}
					if(p - last > 0)
					{
						/* We have found a file name for our redirection */
						pushFile(last, p, rCurrent, &cCurrent->redir, &rTail);
						inRedir = 0;
					}
				}
				
				/* Skip the character */
				last = ++p;
				break;
			case '>':
			case '<':
				if(inSingleQuotes || inDoubleQuotes)
				{
					p++;
					break;
				}
				/* This is a redirection */
				
				/* Create a redirection object */
				rCurrent = safeMalloc(sizeof(struct redirection));
				rCurrent->next = NULL;
				
				/* Determine the redirection mode */
				if(*p == '<')
					rCurrent->mode = READ;
				else if(p[1] == '>')
					rCurrent->mode = WRITE | APPEND;
				else
					rCurrent->mode = WRITE;
				
				/* Look back for an FD number.
				 * The uneaten string before the < or > MUST be
				 * a number and nothing else. This is because
				 * foo 2>bar is different from foo2>bar .
				 */
				fdString = substring(last, p);
				fd = atoi(fdString);
				if((fd > 0 || fdString[0] == '0') && isdigit(fdString[0]))
				{
					/* We found an FD */
					rCurrent->fromfd = fd;
					/* Move last over the FD */
					last = p;
				}
				else
					/* Use the default FD for the selected mode */
					rCurrent->fromfd = rCurrent->mode == READ ? 0 : 1;
				
				/* If we passed an argument or file before, push it.
				 * This is done at this stage so we can eat the FD first.
				 */
				if(inRedir) /* If we WERE in a redirection before */
				{
					/* Push the file we just passed, if any */
					if(p - last > 0)
						pushFile(last, p, rCurrent, &cCurrent->redir, &rTail);
					/* If p - last == 0, we have something like foo > > bar.
					 * Technically that's wrong and we could throw an error, but
					 * we can easily handle it gracefully by doing nothing, which
					 * will effectively ignore all redirection tokens but the last one.
					 */
				}
				else
				{
					/* Push the argument we just passed, if any */
					if(p - last > 0)
						pushArg(last, p, &cCurrent->firstArg, &curTail);
				}
				
				
				/* Set p past the <, > or >> . We expect a file after this */
				p += rCurrent->mode == (WRITE | APPEND) ? 2 : 1;
				last = p;
				inRedir = 1;
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
	
	return cHead;
}
