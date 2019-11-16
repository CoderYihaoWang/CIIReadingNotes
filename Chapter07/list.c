static char rcsid[] = "$Id: list.c 6 2007-01-22 00:45:22Z drhanson $";
#include <stdarg.h>
/// NULL is defined here
/// size_t is also defined here, but it is not used to represent the size
#include <stddef.h>
#include "assert.h"
#include "mem.h"
#include "list.h"
#define T List_T
/// push: to add at the front
T List_push(T list, void *x) {
	T p;
	NEW(p);
	p->first = x;
	p->rest  = list;
	return p;
}
/// note that the last arg must be NULL
T List_list(void *x, ...) {
	va_list ap; /// the handler to use va_list
	T list, *p = &list; /// p: a pointer to a pointer of list node
	va_start(ap, x); /// initialize the va_list
	/// initially, *p is a list which is undefined
	for ( ; x; x = va_arg(ap, void *)) {
		/// this expands to similar to *p = malloc(sizeof **p)
		/// it changes the value of *p, which is, in the first pass, list
		NEW(*p); /// initialising the empty list
		(*p)->first = x; /// x is the pointer to the next arg
		p = &(*p)->rest; /// get the address represented by (*p)->rest, move on
		/// from the second pass, through NEW(*p), the value of (*p)->rest will be changed
	}
	*p = NULL; /// set the ending rest field to NULL
	va_end(ap); /// clean up the va_list
	return list;
}
/// append one list to the end of another
T List_append(T list, T tail) {
	T *p = &list;
	while (*p)
		p = &(*p)->rest;
	*p = tail;
	return list;
}
/// shallow copy
T List_copy(T list) {
	T head, *p = &head;
	for ( ; list; list = list->rest) {
		NEW(*p);
		(*p)->first = list->first;
		p = &(*p)->rest;
	}
	*p = NULL;
	return head;
}
T List_pop(T list, void **x) {
	if (list) {
		T head = list->rest;
		if (x) /// so, List_pop(list, NULL) will discard the value popped
			*x = list->first;
		FREE(list);
		return head;
	} else /// it's not an error to pop an empty list
		return list;
}
T List_reverse(T list) {
	T head = NULL, next;
	for ( ; list; list = next) {
		next = list->rest;
		list->rest = head;
		head = list;
	}
	return head;
}
int List_length(T list) {
	int n;
	for (n = 0; list; list = list->rest)
		n++;
	return n;
}
void List_free(T *list) {
	T next;
	assert(list);
	for ( ; *list; *list = next) {
		next = (*list)->rest;
		FREE(*list);
	}
}
/// it seems the declaration void *cl will not guarantee that the two args are the same
void List_map(T list,
	void apply(void **x, void *cl), void *cl) {
	assert(apply);
	for ( ; list; list = list->rest)
		apply(&list->first, cl); /// by passing cl explicitly to the function apply
}                                        /// it makes sure that apply is called upon the correct arg
/// *end is usually NULL, encountering a NULL pointer indicates the end of a list
void **List_toArray(T list, void *end) {
	int i, n = List_length(list);
	void **array = ALLOC((n + 1)*sizeof (*array)); /// one element more than the underlying list
	for (i = 0; i < n; i++) {
		array[i] = list->first;
		list = list->rest;
	}
	array[i] = end;
	return array;
}
