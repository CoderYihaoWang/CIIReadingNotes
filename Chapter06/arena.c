static char rcsid[] = "$Id: arena.c 6 2007-01-22 00:45:22Z drhanson $";
#include <stdlib.h>
#include <string.h>
#include "assert.h"
#include "except.h"
#include "arena.h"
#define T Arena_T
const Except_T Arena_NewFailed =
	{ "Arena Creation Failed" };
const Except_T Arena_Failed    =
	{ "Arena Allocation Failed" };
#define THRESHOLD 10
struct T {  /// struct Arena_T
	T prev; /// struct Arena_T *
	char *avail; /// the first place in the available area
	char *limit; /// of-the-end of the arena
};
union align {
#ifdef MAXALIGN
	char pad[MAXALIGN];
#else
	int i;
	long l;
	long *lp;
	void *p;
	void (*fp)(void);
	float f;
	double d;
	long double ld;
#endif
};
union header {
	struct T b;
	union align a;
};
static T freechunks;
static int nfree;
T Arena_new(void) {
	T arena = malloc(sizeof (*arena));
	if (arena == NULL)
		RAISE(Arena_NewFailed);
	arena->prev = NULL;
	arena->limit = arena->avail = NULL;
	return arena;
}
void Arena_dispose(T *ap) {
	assert(ap && *ap);
	Arena_free(*ap);
	free(*ap);
	*ap = NULL; /// necessary in order to prevent the problem of dangling pointers
}
void *Arena_alloc(T arena, long nbytes,
	const char *file, int line) {
	assert(arena);
	assert(nbytes > 0);
	nbytes = ((nbytes + sizeof (union align) - 1)/
		(sizeof (union align)))*(sizeof (union align)); /// rounding up
	/// one-pass allocation may not be enough if the memory is allocated from freechunks
	while (nbytes > arena->limit - arena->avail) {
		T ptr;
		char *limit;
		if ((ptr = freechunks) != NULL) { /// return a free chunk, if any
			freechunks = freechunks->prev;
			nfree--;
			limit = ptr->limit;
		} else {
			long m = sizeof (union header) + nbytes + 10*1024; /// larger than requested
			ptr = malloc(m);
			if (ptr == NULL)
				{
					if (file == NULL)
						RAISE(Arena_Failed);
					else
						Except_raise(&Arena_Failed, file, line);
				}
			limit = (char *)ptr + m; /// of the end pointer
		}
		*ptr = *arena; /// copy the header information into the memory chunk allocated
		arena->avail = (char *)((union header *)ptr + 1);
		arena->limit = limit;
		arena->prev  = ptr;
	}
	arena->avail += nbytes;
	return arena->avail - nbytes; 
}
void *Arena_calloc(T arena, long count, long nbytes,
	const char *file, int line) {
	void *ptr;
	assert(count > 0);
	ptr = Arena_alloc(arena, count*nbytes, file, line);
	memset(ptr, '\0', count*nbytes);
	return ptr;
}
void Arena_free(T arena) {
	assert(arena);
	/// release all allocated chunks to freechunks
	while (arena->prev) { /// the first chunk seems not freed
		struct T tmp = *arena->prev;
		if (nfree < THRESHOLD) {
			arena->prev->prev = freechunks;
			freechunks = arena->prev;
			nfree++;
			freechunks->limit = arena->limit;
		} else
			free(arena->prev);
		*arena = tmp;
	}
	assert(arena->limit == NULL);
	assert(arena->avail == NULL);
}
