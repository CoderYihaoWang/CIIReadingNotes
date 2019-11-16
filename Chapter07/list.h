/* $Id: list.h 6 2007-01-22 00:45:22Z drhanson $ */
#ifndef LIST_INCLUDED
#define LIST_INCLUDED
#define T List_T
typedef struct T *T;
/// the T of list is the same as the T of a list node
/// therefore it does not provide length information
/// it seems that the user is able to get the pointer to an element in the middle of a list
/// and pass it as an arg in the functions in this interface
struct T {
	T rest;
	void *first; /// generic in c, however, it allows multiple types to be kept in one list
};
extern T      List_append (T list, T tail);
extern T      List_copy   (T list);
extern T      List_list   (void *x, ...); /// take multiple args to make them a list
extern T      List_pop    (T list, void **x); /// pointer to pointer in order to delete it
extern T      List_push   (T list, void *x);
extern T      List_reverse(T list);
extern int    List_length (T list); /// int is used instead of size_t
extern void   List_free   (T *list);
/// the function pointer apply take two arguments
/// the first is a pointer to pointer to an element kept in list
/// this enables the function to change or delete the underlying item
/// the second is the second operand
/// it seems that if more than one operands are passed in, the rest will be ignored
extern void   List_map    (T list,
	void apply(void **x, void *cl), void *cl);
/// the return type is a pointer to a pointer to an element
/// which can also be seen as an array of pointers
/// however, the user must keep track of the number of elements in the array
/// it seems to be the case in all functions where array is intended to be the return type
extern void **List_toArray(T list, void *end);
#undef T
#endif
