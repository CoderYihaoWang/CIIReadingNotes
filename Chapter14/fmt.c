static char rcsid[] = "$Id: fmt.c 197 2008-09-27 21:59:31Z drhanson $";
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <math.h>
#include "assert.h"
#include "except.h"
#include "fmt.h"
#include "mem.h"
#define T Fmt_T
/// program structure:
///  _______________________________________________________________________________________________
/// |                                   | parse args  | control  |              |                   |
/// |    read in the variable arg list  | dispatch cvt| formats  |   printing   |  where to print   |
/// |-----------------------------------|-------------|----------|--------------|-------------------|
/// |     Fmt_sfmt -> |    Fmt_vsfmt -> |             |          |              |                   |
/// | Fmt_fmt, Fmt_print, Fmt_fprint -> | Fmt_vfmt -> | cvt_X -> | putd/puts -> | outc/insert/append|
/// |   Fmt_string -> |  Fmt_vstring -> |             |          |              |                   |
/// |_________________|_________________|_____________|__________|______________|___________________|

/// buffer to put in chars, easier to manage than a naked char[]
struct buf {
	char *buf; /// the buffer string base
	char *bp;  /// the buffer top
	int size;  /// to detect overflow
};
/// a macro encapsulated in do while will confine the scope of inner 
/// variables, this, however, can not deal with circumstances where 
/// the macro arguments happens to hold the same name with the inner variable
#define pad(n,c) do { int nn = (n); \
	while (nn-- > 0) \
		put((c), cl); } while (0)
/// cvt_X is the conversion function relating to the formatter %X
/// to support more formats, add conversion funcitons here
/// the following functions only convert the variable to a uniformed
/// plain format, the args passed in, such as width and precision,
/// are passed on without being processed
static void cvt_s(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	/// get the argument in va_list and convert it to the type required
	char *str = va_arg(box->ap, char *);
	assert(str);
	Fmt_puts(str, strlen(str), put, cl, flags,
		width, precision);
}
static void cvt_d(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	int val = va_arg(box->ap, int);
	unsigned m;
	char buf[43]; /// estimated maximum length for a string representing a double
	char *p = buf + sizeof buf; /// off-the-end pointer
	/// asymmetrically dealing with INT_MIN
	/// as -INT_MIN == INT_MIN
	if (val == INT_MIN)
		m = INT_MAX + 1U;
	else if (val < 0)
		m = -val;
	else
		m = val;
	do
		*--p = m%10 + '0';
	while ((m /= 10) > 0);
	if (val < 0)
		*--p = '-';
	Fmt_putd(p, (buf + sizeof buf) - p, put, cl, flags,
		width, precision);
}
static void cvt_u(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	unsigned m = va_arg(box->ap, unsigned);
	char buf[43];
	char *p = buf + sizeof buf;
	do
		*--p = m%10 + '0';
	while ((m /= 10) > 0);
	Fmt_putd(p, (buf + sizeof buf) - p, put, cl, flags,
		width, precision);
}
static void cvt_o(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	unsigned m = va_arg(box->ap, unsigned);
	char buf[43];
	char *p = buf + sizeof buf;
	do
		*--p = (m&0x7) + '0'; /// m & 0x7 => m % 8
	while ((m>>= 3) != 0);
	Fmt_putd(p, (buf + sizeof buf) - p, put, cl, flags,
		width, precision);
}
static void cvt_x(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	unsigned m = va_arg(box->ap, unsigned);
	char buf[43];
	char *p = buf + sizeof buf;
	do
		*--p = "0123456789abcdef"[m&0xf];
	while ((m>>= 4) != 0);
	Fmt_putd(p, (buf + sizeof buf) - p, put, cl, flags,
		width, precision);
}
static void cvt_p(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	unsigned long m = (unsigned long)va_arg(box->ap, void*);
	char buf[43];
	char *p = buf + sizeof buf;
	precision = INT_MIN;
	do
		*--p = "0123456789abcdef"[m&0xf];
	while ((m>>= 4) != 0);
	Fmt_putd(p, (buf + sizeof buf) - p, put, cl, flags,
		width, precision);
}
/// cvt_c does not call Fmt_putd or Fmt_puts
/// as only one char is printed
static void cvt_c(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	if (width == INT_MIN)
		width = 0;
	if (width < 0) {
		flags['-'] = 1;
		width = -width;
	}
	if (!flags['-'])
		pad(width - 1, ' ');
	put((unsigned char)va_arg(box->ap, int), cl);
	if ( flags['-'])
		pad(width - 1, ' ');
}
static void cvt_f(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	/// ? why ?
	char buf[DBL_MAX_10_EXP+1+1+99+1];
	if (precision < 0)
		precision = 6;
	if (code == 'g' && precision == 0)
		precision = 1;
	{
		/// make a new fmt representer to pass it on
		static char fmt[] = "%.dd?";
		assert(precision <= 99);
		fmt[4] = code;
		fmt[3] =      precision%10 + '0';
		fmt[2] = (precision/10)%10 + '0';
		sprintf(buf, fmt, va_arg(box->ap, double));
	}
	Fmt_putd(buf, strlen(buf), put, cl, flags,
		width, precision);
}
const Except_T Fmt_Overflow = { "Formatting Overflow" };
/// a map to dispatch formatters to their conversion functions
/// all ascii letters are supported, though some are not printable
static T cvt[256] = {
 /*   0-  7 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*   8- 15 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  16- 23 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  24- 31 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  32- 39 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  40- 47 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  48- 55 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  56- 63 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  64- 71 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  72- 79 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  80- 87 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  88- 95 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  96-103 */ 0,     0, 0, cvt_c, cvt_d, cvt_f, cvt_f, cvt_f,
 /* 104-111 */ 0,     0, 0,     0,     0,     0,     0, cvt_o,
 /* 112-119 */ cvt_p, 0, 0, cvt_s,     0, cvt_u,     0,     0,
 /* 120-127 */ cvt_x, 0, 0,     0,     0,     0,     0,     0
};
char *Fmt_flags = "-+ 0";
/// put one char to the stream cl
/// it requires that the client passes in cl as a stream
static int outc(int c, void *cl) {
	FILE *f = cl;
	return putc(c, f);
}
/// insert one char to the buf
/// raise an error if overflow
/// it requires that the cl is a pointer to a struct buf
static int insert(int c, void *cl) {
	struct buf *p = cl;
	if (p->bp >= p->buf + p->size)
		RAISE(Fmt_Overflow);
	*p->bp++ = c;
	return c;
}
/// insert, but resize the buffer instead of raising an error when insufficient space
static int append(int c, void *cl) {
	struct buf *p = cl;
	if (p->bp >= p->buf + p->size) {
		RESIZE(p->buf, 2*p->size);
		p->bp = p->buf + p->size;
		p->size *= 2;
	}
	*p->bp++ = c;
	return c;
}
/// str: an internal buf provided by cvt_X
/// len: length of buf
/// put: where to put the result, in buf, in file, or in a resized new buf
/// cl: client value, if put is outc, then a file, otherwise a struct buf provided by Fmt_vsfmt
/// flags: an array of 256 chars, to mark whether a flag exists, provided by Fmt_vfmt
/// width: minimum width of printing
/// precision: precision
/// in %s: width is the total space occupied
/// and precision is the length of output string
/// the rest space, if any, is padded with space
/// flag 0 is ignored
/// flag + is ignored
void Fmt_puts(const char *str, int len,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	assert(str);
	assert(len >= 0);
	assert(flags);
	/// default
	if (width == INT_MIN)
		width = 0;
	/// this function is accessible by the user
	/// so it is not guaranteed that the width is above 0
	/// where the width is negative, split it into a width and a flag
	if (width < 0) {
		flags['-'] = 1;
		width = -width;
	}
	if (precision >= 0)
		flags['0'] = 0;
	if (precision >= 0 && precision < len)
		len = precision;
	/// left padding if no -
	if (!flags['-'])
		pad(width - len, ' ');
	{
		int i;
		for (i = 0; i < len; i++)
			put((unsigned char)*str++, cl);
	}
	/// right padding if there is -
	if ( flags['-'])
		pad(width - len, ' ');
}
/// in order to put all format functionalities into a single
/// Fmt_vfmt function, the whole argument list must be passed on
/// when calling it. However, ... is not a real argument and 
/// hence there is no way to pass it to other functions directly
/// So the author wrapped these arguments in a va_list_box and 
/// passed the box by pointer
/// why not just pass on the va_list?
/// it seems one possible reason is that the va_list, declared by
/// the library stdargs.h, might be changed among function calls
/// Fmt_fmt: original version, except for dealing with arg list
void Fmt_fmt(int put(int c, void *), void *cl,
	const char *fmt, ...) {
	va_list_box box;
	va_start(box.ap, fmt);
	Fmt_vfmt(put, cl, fmt, &box);
	va_end(box.ap);
}
/// Fmt_print: output to stdout, similar to printf
void Fmt_print(const char *fmt, ...) {
	va_list_box box;
	va_start(box.ap, fmt);
	Fmt_vfmt(outc, stdout, fmt, &box);
	va_end(box.ap);
}
/// Fmt_fprint: similar to fprintf
void Fmt_fprint(FILE *stream, const char *fmt, ...) {
	va_list_box box;
	va_start(box.ap, fmt);
	Fmt_vfmt(outc, stream, fmt, &box);
	va_end(box.ap);
}
/// Fmt_sfmt: similar to sprintf, however, adds the check to the buffer
/// returns the total chars written
int Fmt_sfmt(char *buf, int size, const char *fmt, ...) {
	int len;
	va_list_box box;
	va_start(box.ap, fmt);
	len = Fmt_vsfmt(buf, size, fmt, &box);
	va_end(box.ap);
	return len;
}
/// this function defines a struct buf as the output object
int Fmt_vsfmt(char *buf, int size, const char *fmt,
	va_list_box *box) {
	struct buf cl;
	assert(buf);
	assert(size > 0);
	assert(fmt);
	cl.buf = cl.bp = buf;
	cl.size = size;
	Fmt_vfmt(insert, &cl, fmt, box);
	insert(0, &cl);
	return cl.bp - cl.buf - 1; /// no trailing '\0', in consistent with sprintf
}
/// this function does not take the buffer as the output string
/// instead, it creates a new string and returns it
/// it is the user's job to free it
char *Fmt_string(const char *fmt, ...) {
	char *str;
	va_list_box box;
	assert(fmt);	
	va_start(box.ap, fmt);
	str = Fmt_vstring(fmt, &box);
	va_end(box.ap);
	return str;
}
char *Fmt_vstring(const char *fmt, va_list_box *box) {
	struct buf cl;
	assert(fmt);
	cl.size = 256;
	cl.buf = cl.bp = ALLOC(cl.size);
	Fmt_vfmt(append, &cl, fmt, box);
	append(0, &cl);
	return RESIZE(cl.buf, cl.bp - cl.buf); /// shrink to fit
}
void Fmt_vfmt(int put(int c, void *cl), void *cl,
	const char *fmt, va_list_box *box) {
	assert(put);
	assert(fmt);
	while (*fmt)
		/// if not a formatter, print it out directly
		/// also deals with double %s witch is interpreted as a valid single %
		if (*fmt != '%' || *++fmt == '%')
			put((unsigned char)*fmt++, cl);
		else
			{
				unsigned char c, flags[256];
				int width = INT_MIN, precision = INT_MIN;
				memset(flags, '\0', sizeof flags); /// convenient way to initialize an array
				/// takes out leading =,-,_ or 0, and put it in the flags map
				if (Fmt_flags) { // "+- 0"
					unsigned char c = *fmt;
					/// strchr: search c in str, return a pointer, NULL if c is not in str
					for ( ; c && strchr(Fmt_flags, c); c = *++fmt) {
						assert(flags[c] < 255);
						flags[c]++;
					}
				}
				/// now get the width
				/// a * indicates that this argument should be read from arg list
				if (*fmt == '*' || isdigit(*fmt)) {
					int n;
					if (*fmt == '*') {
						n = va_arg(box->ap, int);
						assert(n != INT_MIN);
						fmt++;
					} else
						for (n = 0; isdigit(*fmt); fmt++) {
							int d = *fmt - '0';
							assert(n <= (INT_MAX - d)/10);
							n = 10*n + d;
						}
					width = n;
				}
				/// now get the precision
				if (*fmt == '.' && (*++fmt == '*' || isdigit(*fmt))) {
					int n;
					if (*fmt == '*') {
						n = va_arg(box->ap, int);
						assert(n != INT_MIN);
						fmt++;
					} else
						for (n = 0; isdigit(*fmt); fmt++) {
							int d = *fmt - '0';
							assert(n <= (INT_MAX - d)/10);
							n = 10*n + d;
						}
					precision = n;
				}
				/// now get the format specifier
				c = *fmt++;
				assert(cvt[c]);
				/// now convert
				(*cvt[c])(c, box, put, cl, flags, width, precision);
			}
}
/// change or append a new format specifier to the map
/// return the old one's pointer, or NULL if no existing convertor
T Fmt_register(int code, T newcvt) {
	T old;
	assert(0 < code
		&& code < (int)(sizeof (cvt)/sizeof (cvt[0])));
	old = cvt[code];
	cvt[code] = newcvt;
	return old;
}
/// why is this so far from Fmt_puts...
/// ...maybe because it is rather more complex
void Fmt_putd(const char *str, int len,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	int sign;
	assert(str);
	assert(len >= 0);
	assert(flags);
	if (width == INT_MIN)
		width = 0;
	/// get the - flag from the width field
	if (width < 0) {
		flags['-'] = 1;
		width = -width;
	}
	if (precision >= 0)
		flags['0'] = 0;
	if (len > 0 && (*str == '-' || *str == '+')) {
		sign = *str++;
		len--;
	} else if (flags['+'])
		sign = '+';
	else if (flags[' '])
		sign = ' ';
	else
		sign = 0;
	{ int n; /// number of chars to print out
	/// smallest precision
	  if (precision < 0)
	  	precision = 1;
	  if (len < precision)
	  	n = precision;
	  else if (precision == 0 && len == 1 && str[0] == '0')
	  	n = 0;
	  else
	  	n = len;
	  if (sign)
	  	n++;
	  if (flags['-']) {
	  	if (sign)
			put(sign, cl);
	  } else if (flags['0']) {
	  	if (sign)
			put(sign, cl);
	  	pad(width - n, '0');
	  } else {
	  	pad(width - n, ' ');
	  	if (sign)
			put(sign, cl);
	  }
	  pad(precision - len, '0');
	  {
	  	int i;
	  	for (i = 0; i < len; i++)
	  		put((unsigned char)*str++, cl);
	  }
	  if (flags['-'])
	  	pad(width - n, ' '); 
	}
}
