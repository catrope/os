#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGESIZE 4096

void *page;
int readFD, writeFD;
pid_t otherPID;
int myNum;
int havePage;

void segvHandler(int sig, siginfo_t *info, void *context) {
	size_t bytesRead, bytesLeft;
	char *p;
	if (info->si_addr < page || info->si_addr >= page + PAGESIZE) {
		/* Segfault due to something else */
		/* Restore default SIGSEGV action */
		signal(SIGSEGV, SIG_DFL);
		return;
	}
	
	if(havePage)
	{
		puts("WTF");
		raise(SIGHUP);
	}
	/* Obtain the page */
	/* Signal the other process */
	kill(otherPID, SIGUSR1);
	/* Unprotect the page, must happen before reading it in */
	mprotect(page, PAGESIZE, PROT_WRITE | PROT_READ);
	/* Read the page from the pipe */
	bytesRead = 0;
	bytesLeft = PAGESIZE;
	p = page;
	do {
		bytesRead = read(readFD, p + PAGESIZE - bytesLeft, bytesLeft);
		if(bytesRead < 0)
		{
			perror("read");
			exit(errno);
		}
		bytesLeft -= bytesRead;
	} while(bytesLeft > 0);
	havePage = 1;
}

void usr1Handler(int sig)
{
	size_t bytesWritten, bytesLeft;
	char *p;
	
	if(!havePage)
	{
		puts("WTF2");
		raise(SIGHUP);
	}
	/* The other process wants the page */
	/* Write it to the pipe */
	bytesWritten = 0;
	bytesLeft = PAGESIZE;
	p = page;
	do {
		bytesWritten = write(writeFD, p + PAGESIZE - bytesLeft, bytesLeft);
		if(bytesWritten < 0)
		{
			perror("write");
			exit(errno);
		}
		bytesLeft -= bytesWritten;
	} while(bytesLeft > 0);
	/* Protect the page. Must happen after writing it out */
	mprotect(page, PAGESIZE, PROT_NONE);
	havePage = 0;
}

int main(void) {
	int ok, child, i, status, fds[4];
	struct sigaction segv, usr1;
	int *turn;
	
	/* Set up a pipe and fork */
	pipe(fds);
	pipe(&fds[2]);
	child = fork();
	if(child < 0)
	{
		perror("fork");
		exit(errno);
	}
	
	/* Get the PID of the other process */
	otherPID = child ? child : getppid();
	myNum = !child;
	
	/* Close unneeded pipes and set up FDs */
	if(child)
	{
		/* Parent */
		readFD = fds[0];
		writeFD = fds[3];
		close(fds[1]);
		close(fds[2]);
	}
	else
	{
		/* Child */
		readFD = fds[2];
		writeFD = fds[1];
		close(fds[0]);
		close(fds[3]);
	}
	
	/* Install SIGSEGV handler */
	segv.sa_sigaction = segvHandler;
	segv.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&segv.sa_mask);
	sigaddset(&segv.sa_mask, SIGUSR1);
	sigaction(SIGSEGV, &segv, NULL);
	
	/* Install SIGUSR1 handler */
	usr1.sa_handler = usr1Handler;
	usr1.sa_flags = SA_RESTART;
	sigemptyset(&usr1.sa_mask);
	sigaddset(&usr1.sa_mask, SIGSEGV);
	sigaction(SIGUSR1, &usr1, NULL);
	
	/* Allocate buffer */
	ok = posix_memalign(&page, PAGESIZE, PAGESIZE);
	if (ok != 0) {
		perror("Couldn't malloc an aligned page");
		exit(errno);
	}
	
	/* Initially, the parent owns the page */
	turn = page;
	if(myNum)
	{
		mprotect(page, PAGESIZE, PROT_NONE);
		havePage = 0;
	}
	else
	{
		mprotect(page, PAGESIZE, PROT_READ | PROT_WRITE);
		*turn = myNum;
		havePage = 1;
	}
	
	for(i = 0; i < 100; i++)
	{
		
		while(*turn != myNum);
		printf("%d: %s\n", getpid(), myNum == 1 ? "Pong" : "Ping");
		*turn = !myNum;
	}
	
	/* Parent hands over the page and waits for child */
	if(child)
	{
		signal(SIGUSR1, SIG_IGN);
		if(havePage)
			usr1Handler(0);
		waitpid(child, &status, 0);
	}
	
	return EXIT_SUCCESS;
}
