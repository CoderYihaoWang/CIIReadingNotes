static char rcsid[] = "$Id: stack.c 6 2007-01-22 00:45:22Z drhanson $";
#include <stddef.h>
#include "assert.h"
#include "mem.h"
#include "stack.h"
#define T Stack_T
struct T { /// here only, T stands for Stack_T, otherwise it stands for Stack_T*
	int count;
	struct elem {
		void *x; /// generic in c
		struct elem *link;
	} *head;
};
T Stack_new(void) {
	T stk;
	NEW(stk); /// similar to: stk = malloc((long)sizeof *stk), but with exception handling
	stk->count = 0;
	stk->head = NULL;
	return stk;
}
int Stack_empty(T stk) {
	assert(stk);
	return stk->count == 0;
}
/// Stack_T only keeps a stack of void pointers
/// users must keep track of the types of values kept in the stack
/// and convert them back and forth
/// if the values in the stack can be accessed elsewhere
/// it is possible that some of the pointers kept in the stack become dangling
void Stack_push(T stk, void *x) {
	struct elem *t;
	assert(stk);
	NEW(t);
	t->x = x;
	t->link = stk->head;
	stk->head = t;
	stk->count++;
}
void *Stack_pop(T stk) {
	void *x;
	struct elem *t;
	assert(stk);
	assert(stk->count > 0);
	t = stk->head; /// the original stk->head must be kept in order to free it
	stk->head = t->link;
	stk->count--;
	x = t->x;
	FREE(t);
	return x;
}
/// a pointer to pointer to Stack_T is used, otherwise *stk will not be set to NULL,
/// hence will cause a problem of dangling pointer
void Stack_free(T *stk) { 
	struct elem *t, *u;
	assert(stk && *stk);
	for (t = (*stk)->head; t; t = u) {
		u = t->link;
		FREE(t);
	}
	FREE(*stk);
}
