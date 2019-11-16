/* $Id: assert.h 6 2007-01-22 00:45:22Z drhanson $ */
/// declaring assert as macros makes it more convenient to 
/// choose whether to keep assertion functions during compilation
/// it is also a requirement by the standard
/// also, in order to provide useful __FILE__, __LINE__ and __func__,
/// it must not be a function

/// making it an expression and not a statement makes it possible
/// to use it in wider circumstances
/// eg. for (int i = 0; assert(e); i = get_next_i()) {...}
/// if assert is a statement, syntax errors will occur
#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0) /// discard the value, make it useless, eg. cannot 'if (assert(e))'
#else
#include "except.h"
extern void assert(int e); /// ?? but why make it a function as well ??
#define assert(e) ((void)((e)||(RAISE(Assert_Failed),0))) /// if e != 0, raise an error, discard the value
#endif
