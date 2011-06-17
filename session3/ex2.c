#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>

#define PAGESIZE 4096

void *page;

void handler(int sig, siginfo_t *info, void *context) {
	if (info.si_addr != page) {
		/* Segfault due to something else */
		/* TODO: Test */
		signal(SIGSEGV, SIG_DFL);
		return;
	}
	/* Stuff */
	mprotect(page, 1024, PROT_WRITE | PROT_READ);
}

int main(void) {
	int ok;
	struct sigaction a;
	
	/* TODO: Set up pipe and fork */
	
	/* Install SIGSEGV handler */
	a.sa_sigaction = handler;
	a.sa_flags = SA_SIGACTION;
	sigemptyset(&a.sa_mask);
	sigaction(SIGSEGV, &a, NULL);
	
	/* Allocate buffer */
	ok = posix_memalign(&page, PAGESIZE, PAGESIZE);
	if (ok != 0) {
		perror("Couldn't malloc an aligned page");
		exit(errno);
	}
	
	/* TODO: More */
}
