/* $Id: fmt.h 197 2008-09-27 21:59:31Z drhanson $ */
#ifndef FMT_INCLUDED
#define FMT_INCLUDED
#include <stdarg.h>
#include <stdio.h>
#include "except.h"
typedef struct va_list_box {
	va_list ap;
} va_list_box;
#define T Fmt_T
/// Fmt_T is a pointer to a function which returns no value,
/// and takes an int, a pointer to va_list_box, 
/// a function taking an int and a void pointer and returning an int, a void pointer
/// an array of unsigned char, and two ints as the arguments
/// -- the type for conversion functions
typedef void (*T)(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[256], int width, int precision);
extern char *Fmt_flags;
extern const Except_T Fmt_Overflow;
extern void Fmt_fmt (int put(int c, void *cl), void *cl,
	const char *fmt, ...);
extern void Fmt_vfmt(int put(int c, void *cl), void *cl,
	const char *fmt, va_list_box *box);
extern void Fmt_print (const char *fmt, ...);
extern void Fmt_fprint(FILE *stream,
	const char *fmt, ...);
extern int Fmt_sfmt   (char *buf, int size,
	const char *fmt, ...);
extern int Fmt_vsfmt(char *buf, int size,
	const char *fmt, va_list_box *box);
extern char *Fmt_string (const char *fmt, ...);
extern char *Fmt_vstring(const char *fmt, va_list_box *box);
/// take a conversion function, and return the previous one
extern T Fmt_register(int code, T cvt);
/// two conversion functions, Fmt_putd is used in conversion functions
/// where a single value is to be printed
/// Fmt_puts is used where a string needs to be printed
extern void Fmt_putd(const char *str, int len,
	int put(int c, void *cl), void *cl,
	unsigned char flags[256], int width, int precision);
extern void Fmt_puts(const char *str, int len,
	int put(int c, void *cl), void *cl,
	unsigned char flags[256], int width, int precision);
#undef T
#endif
