#ifndef COMMAND_H
#define COMMAND_H

#include <sys/types.h>
#define READ 1
#define WRITE 2
#define APPEND 4
#define PIPE 8

#define BACKGROUND 1
#define PIPED 2

struct redirection
{
	int fromfd; /* FD being redirected */
	char *filename; /* Filename to redirect to, or NULL if redirecting to an FD */
	int tofd; /* FD to redirect to. Only used if filename is NULL */
	int mode; /* Bitmap of READ, WRITE, APPEND, PIPE */
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
	int mode; /* Bitmap of BACKGROUND, PIPED */
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

/**
 * Recursively free a list of argument structures
 */
extern void freeArgumentList(struct argument *head);

/**
 * Recursively free a list of redirection structures
 */
extern void freeRedirectionList(struct redirection *head);

/**
 * Recursively free a list of command structures
 */
extern void freeCommandList(struct command *head);

/**
 * Free one command structure and its members, but don't free the the rest
 * of the list
 */
extern void freeCommand(struct command *cmd);

#endif /* COMMAND_H */
