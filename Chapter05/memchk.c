static char rcsid[] = "$Id: memchk.c 6 2007-01-22 00:45:22Z drhanson $";
/// this API is to check whether there are dangerous usages of memory allocation
/// such as freeing a pointer which is not allocated by allocs
/// or freeing a pointer which has already been freed (but not set to NULL)
#include <stdlib.h>
#include <string.h>
#include "assert.h"
#include "except.h"
#include "mem.h"
union align { /// union align: the size of align can contain any of its components
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
/// sizeof t / sizeof (t[0]) - 1
/// => length of array t - 1
/// unsigned long & length - 1: trim the integer value to the last several bits
/// so that the max value does not exceed the length of the array
/// the size of the array needs to be 2's powers
/// here, the address is converted into a long int to get hash
#define hash(p, t) (((unsigned long)(p)>>3) & \
	(sizeof (t)/sizeof ((t)[0])-1))
#define NDESCRIPTORS 512 /// number of descriptors allocated at one time
#define NALLOC ((4096 + sizeof (union align) - 1)/ \
	(sizeof (union align)))*(sizeof (union align))
const Except_T Mem_Failed = { "Allocation Failed" };
static struct descriptor {
	struct descriptor *free;
	struct descriptor *link;
	const void *ptr;
	long size;
	const char *file;
	int line;
} *htab[2048];
static struct descriptor freelist = { &freelist }; /// a cirlular list
static struct descriptor *find(const void *ptr) {
	struct descriptor *bp = htab[hash(ptr, htab)];
	while (bp && bp->ptr != ptr)
		bp = bp->link;
	return bp;
}
/// if the pointer is not created by the functions in the library
/// or if the pointer is already freed, raise an error
/// else, put it in the freelist list
void Mem_free(void *ptr, const char *file, int line) {
	if (ptr) {
		struct descriptor *bp;
		if (((unsigned long)ptr)%(sizeof (union align)) != 0
		|| (bp = find(ptr)) == NULL || bp->free)
			Except_raise(&Assert_Failed, file, line);
		/// insertion to a circular list
		bp->free = freelist.free;
		freelist.free = bp;
	}
}
void *Mem_resize(void *ptr, long nbytes,
	const char *file, int line) {
	struct descriptor *bp;
	void *newptr;
	assert(ptr);
	assert(nbytes > 0);
	if (((unsigned long)ptr)%(sizeof (union align)) != 0
	|| (bp = find(ptr)) == NULL || bp->free)
		Except_raise(&Assert_Failed, file, line);
	newptr = Mem_alloc(nbytes, file, line);
	memcpy(newptr, ptr,
		nbytes < bp->size ? nbytes : bp->size);
	Mem_free(ptr, file, line);
	return newptr;
}
void *Mem_calloc(long count, long nbytes,
	const char *file, int line) {
	void *ptr;
	assert(count > 0);
	assert(nbytes > 0);
	ptr = Mem_alloc(count*nbytes, file, line);
	memset(ptr, '\0', count*nbytes); /// initializing
	return ptr;
}
/// allocate a piece of memory for a descriptor
/// NDESCRIPTORS descriptors are allocated at one time
/// each time one descriptor is returned
/// a next chunck is allocated when the previous chunck is used up
static struct descriptor *dalloc(void *ptr, long size,
	const char *file, int line) {
	static struct descriptor *avail;
	static int nleft;
	if (nleft <= 0) {
		avail = malloc(NDESCRIPTORS*sizeof (*avail));
		if (avail == NULL)
			return NULL;
		nleft = NDESCRIPTORS;
	}
	avail->ptr  = ptr;
	avail->size = size;
	avail->file = file;
	avail->line = line;
	avail->free = avail->link = NULL;
	nleft--;
	return avail++;
}
void *Mem_alloc(long nbytes, const char *file, int line){
	struct descriptor *bp;
	void *ptr;
	assert(nbytes > 0);
	/// round nbytes up to an alignment boundary
	/// a / b * b => rounding down
	/// (a + b - 1) / b * b => rounding up
	nbytes = ((nbytes + sizeof (union align) - 1)/
		(sizeof (union align)))*(sizeof (union align));
	for (bp = freelist.free; bp; bp = bp->free) {
		/// searching in the freelists, to see if there is an available one
		/// using first fit principle
		if (bp->size > nbytes) {
			/// carve out a piece from the beginning, return it as the memory allocated
			/// allocate a new descriptor for the rest of the chunck, add it to htab
			bp->size -= nbytes;
			ptr = (char *)bp->ptr + bp->size;
			if ((bp = dalloc(ptr, nbytes, file, line)) != NULL) {
				unsigned h = hash(ptr, htab);
				bp->link = htab[h];
				htab[h] = bp;
				return ptr;
			} else
				{
					if (file == NULL)
						RAISE(Mem_Failed);
					else
						Except_raise(&Mem_Failed, file, line);
				}
		}
		/// reaching the end of freelist, haven't found an available one
		/// then make a new one, and append it to the end of free list
		if (bp == &freelist) {
			struct descriptor *newptr;
			/// to allocate a memory piece which is 4096 rounded up bytes larger than necessary
			/// to avoid that the memory pieces get too splitted too quickly
			if ((ptr = malloc(nbytes + NALLOC)) == NULL
			||  (newptr = dalloc(ptr, nbytes + NALLOC,
					__FILE__, __LINE__)) == NULL)
				{
					if (file == NULL)
						RAISE(Mem_Failed);
					else
						Except_raise(&Mem_Failed, file, line);
				}
			newptr->free = freelist.free;
			freelist.free = newptr;
		}
	}
	assert(0);
	return NULL;
}
