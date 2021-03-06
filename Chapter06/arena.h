/* $Id: arena.h 6 2007-01-22 00:45:22Z drhanson $ */
#ifndef ARENA_INCLUDED
#define ARENA_INCLUDED
#include "except.h"
#define T Arena_T
typedef struct T *T;
extern const Except_T Arena_NewFailed;
extern const Except_T Arena_Failed;
extern T    Arena_new    (void);
extern void Arena_dispose(T *ap); /// disposing methods should pass in the the pointer to pointer of objects
extern void *Arena_alloc (T arena, long nbytes,
	const char *file, int line);
extern void *Arena_calloc(T arena, long count,
	long nbytes, const char *file, int line);
extern void  Arena_free  (T arena);
#undef T
#endif
