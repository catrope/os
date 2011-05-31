#ifndef COMMAND_H
#define COMMAND_H

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

/**
 * Parse a command line, recognizing pipes and I/O redirection, and segmenting
 * argument lists.
 * @param commandLine Command line
 * @param redirs Set to a pointer to the head of a linked list of redirections
 * @return Pointer to the head of a linked list of commands
 */
extern struct command *parseCommandLine(const char *commandLine, struct redirection **redirs);

#endif /* COMMAND_H */
