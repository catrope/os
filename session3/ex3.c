#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/file.h>

#define PAGESIZE 4096

void *page;
int readFD, writeFD;
pid_t otherPID;
int myNum, fds[4];
int dirty; /* 0: both up-to-date, 1: mine up-to-date and dirty, 2: theirs up-to-date and dirty */

void segvHandler(int sig, siginfo_t *info, void *context) {
	size_t bytesRead, bytesLeft;
	char *p;
	if (info->si_addr < page || info->si_addr >= page + PAGESIZE) {
		/* Segfault due to something else */
		/* Restore default SIGSEGV action */
		signal(SIGSEGV, SIG_DFL);
		return;
	}
	
	/* Prevent both processes from going into the critical section below by having them lock each end of the pipe */
	flock(fds[myNum], LOCK_EX);
	
	if(dirty == 0)
	{
		/* Trying to write to a clean page */
		/* Mark the page as dirty and ours */
		dirty = 1;
		/* Inform the other process the page is dirty */
		kill(otherPID, SIGUSR2);
		/* Unprotect the page */
		mprotect(page, PAGESIZE, PROT_READ | PROT_WRITE);
	}
	else if(dirty == 2)
	{
		/* Obtain the page */
		/* Signal the other process */
		kill(otherPID, SIGUSR1);
		/* Unprotect the page, must happen before reading it in */
		mprotect(page, PAGESIZE, PROT_READ | PROT_WRITE);
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
		mprotect(page, PAGESIZE, PROT_READ);
		dirty = 0;
	}
	flock(fds[myNum], LOCK_UN);
}

void usr1Handler(int sig)
{
	size_t bytesWritten, bytesLeft;
	char *p;
	
	/* The other process wants the page */
	/* Write it to the pipe */
	bytesWritten = 0;
	bytesLeft = PAGESIZE;
	p = page;
	/* Protect the page from writing. We do this here because we're paranoid that the page might have been read-protected before. */
	mprotect(page, PAGESIZE, PROT_READ);
	do {
		bytesWritten = write(writeFD, p + PAGESIZE - bytesLeft, bytesLeft);
		if(bytesWritten < 0)
		{
			perror("write");
			exit(errno);
		}
		bytesLeft -= bytesWritten;
	} while(bytesLeft > 0);
	dirty = 0;
}

void usr2Handler(int sig)
{
	/* The other process is writing to the page */
	dirty = 2;
	mprotect(page, PAGESIZE, PROT_NONE);
}

int main(void) {
	int ok, child, i, status;
	struct sigaction segv, usr1, usr2;
	int *turn;
	
	/* Allocate buffer */
	ok = posix_memalign(&page, PAGESIZE, PAGESIZE);
	if (ok != 0) {
		perror("Couldn't malloc an aligned page");
		exit(errno);
	}
	
	/* Initialize stuff */
	turn = page;
	dirty = 0;
	mprotect(page, PAGESIZE, PROT_READ);
	
	/* Install SIGSEGV handler */
	segv.sa_sigaction = segvHandler;
	segv.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&segv.sa_mask);
	sigaddset(&segv.sa_mask, SIGUSR1);
	sigaddset(&segv.sa_mask, SIGUSR2);
	sigaction(SIGSEGV, &segv, NULL);
	
	/* Install SIGUSR1 handler */
	usr1.sa_handler = usr1Handler;
	usr1.sa_flags = SA_RESTART;
	sigemptyset(&usr1.sa_mask);
	sigaddset(&usr1.sa_mask, SIGSEGV);
	sigaddset(&usr1.sa_mask, SIGUSR2);
	sigaction(SIGUSR1, &usr1, NULL);
	
	/* Install SIGUSR2 handler */
	usr2.sa_handler = usr2Handler;
	usr2.sa_flags = SA_RESTART;
	sigemptyset(&usr2.sa_mask);
	sigaddset(&usr2.sa_mask, SIGSEGV);
	sigaddset(&usr2.sa_mask, SIGUSR1);
	sigaction(SIGUSR2, &usr2, NULL);
	
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
	/* We need fds[0] and fds[1] for locking so don't close those */
	if(child)
	{
		/* Parent */
		readFD = fds[0];
		writeFD = fds[3];
		close(fds[2]);
	}
	else
	{
		/* Child */
		readFD = fds[2];
		writeFD = fds[1];
		close(fds[3]);
	}
	
	for(i = 0; i < 10; i++)
	{
		
		while(*turn != myNum);
		printf("%d: %s #%d\n", getpid(), myNum == 1 ? "Pong" : "Ping", i);
		*turn = !myNum;
	}
	
	/* Parent hands over the page and waits for child */
	if(child)
	{
		signal(SIGUSR1, SIG_IGN);
		signal(SIGUSR2, SIG_IGN);
		usr1Handler(0);
		waitpid(child, &status, 0);
	}
	
	return EXIT_SUCCESS;
}
