#include <stdio.h>
#include "util.h"

void *safeMalloc(size_t s)
{
	void *r = malloc(s);
	if(!r)
	{
		printf("Memory allocation failed. Tried to allocate %d bytes.\n", (int)s);
		exit(EXIT_FAILURE);
	}
	return r;
}
