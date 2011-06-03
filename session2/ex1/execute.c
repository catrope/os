#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include "util.h"
#include "execute.h"

/**
 * Resolve a list of redirections, translating each file name to an open FD
 * @param r Linked list of redirections to resolve
 */
static void resolveRedirections(struct redirection *redirs)
{
	struct redirection *r;
	int tofd, flags;
	for(r = redirs; r; r = r->next)
	{
		if(r->filename)
		{
			/* Build flags */
			flags = 0;
			if(r->mode & READ)
				flags |= O_RDONLY;
			else if(r->mode & WRITE)
				flags |= O_WRONLY | O_CREAT;
			if(r->mode & APPEND)
				flags |= O_APPEND;
			else if(r->mode & WRITE)
				flags |= O_TRUNC;
			
			/* Open file */
			tofd = open(r->filename, flags, 0777);
			if(tofd < 0)
			{
				/* open failed, abort */
				perror(r->filename);
				exit(EXIT_FAILURE);
			}
			r->tofd = tofd;
		}
	}
}

static void setupRedirections(struct redirection *redirs)
{
	struct redirection *r;
	for(r = redirs; r; r = r->next)
	{
		if(r->fromfd != r->tofd)
		{
			if(dup2(r->tofd, r->fromfd) < 0)
			{
				/* dup2 failed, abort */
				perror("dup2");
				exit(EXIT_FAILURE);
			}
			if(r->tofd > 2) /* Don't close stdin, stdout or stderr */
				close(r->tofd);
		}
	}
}

/**
 * Close pipes used for other commands
 * @param cmds Linked list of commands
 * @param current Current command. Will be skipped
 */
static void closeOtherPipes(struct command *cmds, struct command *current)
{
	struct command *c;
	struct redirection *r;
	for(c = cmds; c; c = c->next)
	{
		if(c == current)
			continue;
		for(r = c->redir; r; r = r->next)
			if(r->mode & PIPE)
				close(r->tofd);
	}
}

void executeCommand(struct command *c)
{
	int i, first, n;
	int fds[2], nextHasReadPipe = 0;
	char **args;
	pid_t child;
	struct command *p;
	struct argument *a;
	struct redirection *tail, *r;
	
	/* Walk through the list and:
	 * 1) resolve redirections to files
	 * 2) set up pipes and add them as redirections
	 */
	for(p = c; p; p = p->next)
	{
		if(p->redir)
			resolveRedirections(p->redir);
		
		/* Pipe setup */
		tail = NULL;
		if(nextHasReadPipe)
		{
			/* Set up the read end of the previous pipe */
			r = safeMalloc(sizeof(struct redirection));
			r->filename = NULL;
			r->mode = READ | PIPE;
			r->fromfd = 0;
			r->tofd = fds[0];
			r->next = NULL;
			
			/* Add to the redirections list */
			if(!p->redir)
				p->redir = tail = r;
			else
			{
				/* Find the tail */
				tail = p->redir;
				while(tail->next)
					tail = tail->next;
				tail->next = r;
				tail = r;
			}
			
			nextHasReadPipe = 0;
		}
		if(p->next && (p->mode & PIPED))
		{
			
			/* Create pipe */
			pipe(fds);
			
			/* Set up the write end */
			r = safeMalloc(sizeof(struct redirection));
			r->filename = NULL;
			r->mode = WRITE | PIPE;
			r->fromfd = 1;
			r->tofd = fds[1];
			r->next = NULL;
			
			/* Add to the redirections list */
			if(!p->redir)
				p->redir = r;
			else
			{
				if(!tail)
				{
					/* Find the tail */
					tail = p->redir;
					while(tail->next)
						tail = tail->next;
				}
				tail->next = r;
			}
			nextHasReadPipe = 1;
		}
	}

	
	/* Now walk through the list and fork processes */
	i = 0;
	first = 1;
	for(p = c; p; p = p->next)
	{
		child = fork();
		if(child < 0)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		if(child > 0)
			/* Parent process */
			p->pid = child;
		else
		{
			/* Child process */
			if(p->redir)
				setupRedirections(p->redir);
			closeOtherPipes(c, p);
			
			/* Count the number of arguments */
			n = 0;
			for(a = c->firstArg; a; a = a->next)
				n++;
			/* Allocate and fill args array */
			args = safeMalloc((n + 1)*sizeof(char *));
			for(n = 0, a = p->firstArg; a; n++, a = a->next)
				args[n] = a->s;
			args[n] = NULL;
			
			execvp(args[0], args);
			
			/* If we're alive here, execvp() failed */
			perror(args[0]);
			exit(EXIT_FAILURE);
		}
		first = 0;
	}
	
	/* Close pipes in the parent */
	closeOtherPipes(c, NULL);
}

void waitForChildren(struct command *c)
{
	struct command *p;
	int status;
	for(p = c; p; p = p->next)
		while(waitpid(p->pid, &status, 0) < 0 && errno == EINTR);
}
