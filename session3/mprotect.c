#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>

#define PAGESIZE 4096  /* type in shell: getconf PAGE_SIZE */

void *address;

void MemoryWrite(int a) {
    printf ("Memory was protected.\n");
    mprotect(address, 1024, PROT_WRITE | PROT_READ);
}

int main(void) {
  char *arr;
  int ok;
    
  signal(SIGSEGV, MemoryWrite);
    
  /* Allocate a buffer; it will have the default
   * protection of PROT_READ|PROT_WRITE. 
   */
  ok = posix_memalign(&address, PAGESIZE, 1024);
  if (ok != 0) {
    perror("Couldn't malloc 1024 page-aligned bytes");
    exit(errno);
  }
    
  arr = (char*)address;
  arr[666] = 42;        /* Write; ok */
    
  /* Mark the buffer read-only. */
  if (mprotect(address, 1024, PROT_READ)) {
    perror("Couldn't mprotect");
    exit(errno);
  }
    
  printf ("val = %d\n", arr[666]);   /* Read; ok */

  arr[666] = 2*arr[666];        /* Write; generates SIGSEGV */

  printf ("val = %d\n", arr[666]);
    
  return EXIT_SUCCESS;
}
