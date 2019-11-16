static char rcsid[] = "$Id: mem.c 6 2007-01-22 00:45:22Z drhanson $";
#include <stdlib.h>
#include <stddef.h>
#include "assert.h"
#include "except.h"
#include "mem.h"
const Except_T Mem_Failed = { "Allocation Failed" };
/// similar to malloc, but raises an exception when allcation fails
void *Mem_alloc(long nbytes, const char *file, int line){
	void *ptr;
	assert(nbytes > 0);
	ptr = malloc(nbytes);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
void *Mem_calloc(long count, long nbytes,
	const char *file, int line) {
	void *ptr;
	assert(count > 0);
	assert(nbytes > 0);
	ptr = calloc(count, nbytes);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
void Mem_free(void *ptr, const char *file, int line) {
	if (ptr) /// avoids the problem of freeing a pointer which has already been freed
		free(ptr); /// it should not be called by user since it does not set ptr to 0
}
void *Mem_resize(void *ptr, long nbytes,
	const char *file, int line) {
	assert(ptr);
	assert(nbytes > 0);
	ptr = realloc(ptr, nbytes);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
