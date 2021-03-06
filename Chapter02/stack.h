/* $Id: stack.h 6 2007-01-22 00:45:22Z drhanson $ */
#ifndef STACK_INCLUDED
#define STACK_INCLUDED
#define T Stack_T
typedef struct T *T; /// T and T are of different name space
extern T     Stack_new  (void);
extern int   Stack_empty(T stk);
extern void  Stack_push (T stk, void *x);
extern void *Stack_pop  (T stk);
extern void  Stack_free (T *stk); /// stk: a pointer to pointer to Stack_T
#undef T
#endif
