#ifndef COMMAND_H
#define COMMAND_H

#include <sys/types.h>
enum redirMode { IN, OUT, OUTAPPEND };

#define BACKGROUND 1
#define PIPED 2

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
	struct argument *firstArg; /* Pointer to the head of the argument list */
	struct redirection *redir; /* Pointer to the head of the redirection list or NULL if no redirections */
	int mode; /* Bitmap with BACKGROUND, PIPED or both or neither */
	pid_t pid; /* PID once started */
	struct command *next;
};

/**
 * Parse a command line, recognizing pipes and I/O redirection, and segmenting
 * argument lists.
 * @param commandLine Command line
 * @return Pointer to the head of a linked list of commands
 */
extern struct command *parseCommandLine(const char *commandLine);

#endif /* COMMAND_H */
