static const char *rcsid = "$Id: text.c 197 2008-09-27 21:59:31Z drhanson $";
#include <string.h>
#include <limits.h>
#include "assert.h"
#include "fmt.h"
#include "text.h"
#include "mem.h"
#define T Text_T
/// index converter
#define idx(i, len) ((i) <= 0 ? (i) + (len) : (i) - 1)
/// to test whether it is possible to append a string with the length of n
/// to the end of string s
#define isatend(s, n) ((s).str+(s).len == current->avail\
	&& current->avail + (n) <= current->limit)
/// whether the string starting from the ith char of s is equal to t
/// assuming the length of string &(s).str[i] is at least the same as t
#define equal(s, i, t) \
	(memcmp(&(s).str[i], (t).str, (t).len) == 0)
/// a snap shot of the chunk status
struct Text_save_T {
	struct chunk *current;
	char *avail;
};
static char cset[] =
	"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
	"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
	"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
	"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
	"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
	"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
	"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
	"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
	"\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217"
	"\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237"
	"\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257"
	"\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277"
	"\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
	"\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337"
	"\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357"
	"\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"
	;
const T Text_cset   = { 256, cset };
const T Text_ascii  = { 127, cset };
const T Text_ucase  = {  26, cset + 'A' };
const T Text_lcase  = {  26, cset + 'a' };
const T Text_digits = {  10, cset + '0' };
const T Text_null   = {   0, cset };
/// a linked list of memory chunks which may contain 
static struct chunk {
	struct chunk *link;
	char *avail;
	char *limit;
} head = { NULL, NULL, NULL }, *current = &head;
/// allocate a chunck of memory to store a string
/// returns the begginging address of the chunk
/// alloc is invoked on the global struct current only
static char *alloc(int len) {
	assert(len >= 0);
	/// if there is not enough space in the current chunk
	/// then allocate a new chunck, and set the pointer current to it
	if (current->avail + len > current->limit) {
		current = current->link = 
			ALLOC(sizeof (*current) + 10*1024 + len);
			/// the string is attached to the end of the discriptor
		current->avail = (char *)(current + 1);
		current->limit = current->avail + 10*1024 + len;
		current->link = NULL;
	}
	current->avail += len;
	return current->avail - len;
}
/// returns the positive index
int Text_pos(T s, int i) {
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	assert(i >= 0 && i <= s.len);
	return i + 1;
}
/// en-box a string
T Text_box(const char *str, int len) {
	T text;
	assert(str);
	assert(len >= 0);
	text.str = str;
	text.len = len;
	return text;
}
/// sub-string, the sub-string returned is a wraped string
/// in which the actual string is the same as str in s
T Text_sub(T s, int i, int j) {
	T text;
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	text.len = j - i;
	text.str = s.str + i;
	return text;
}
/// copy a string into a box
T Text_put(const char *str) {
	T text;
	assert(str);
	text.len = strlen(str);
	text.str = memcpy(alloc(text.len), str, text.len);
	return text;
}
/// copy the string in a box to a native string
char *Text_get(char *str, int size, T s) {
	assert(s.len >= 0 && s.str);
	if (str == NULL)
		str = ALLOC(s.len + 1);
	else
		assert(size >= s.len + 1);
	memcpy(str, s.str, s.len);
	str[s.len] = '\0';
	return str;
}
/// duplicate s n times
T Text_dup(T s, int n) {
	assert(s.len >= 0 && s.str);
	assert(n >= 0);
	if (n == 0 || s.len == 0)
		return Text_null;
	if (n == 1)
		return s;
	{
		T text;
		char *p;
		text.len = n*s.len;
		/// if the space in the current chunk is enough
		/// then allocate the duplicates at the end ...
		if (isatend(s, text.len - s.len)) {
			text.str = s.str;
			p = alloc(text.len - s.len);
			n--;
		} else
			/// ... otherwise allocate the duplicates in a new chunck
			text.str = p = alloc(text.len);
		for ( ; n-- > 0; p += s.len)
			memcpy(p, s.str, s.len);
		return text;
	}
}
T Text_cat(T s1, T s2) {
	assert(s1.len >= 0 && s1.str);
	assert(s2.len >= 0 && s2.str);
	if (s1.len == 0)
		return s2;
	if (s2.len == 0)
		return s1;
	if (s1.str + s1.len == s2.str) {
		s1.len += s2.len;
		return s1;
	}
	{
		T text;
		text.len = s1.len + s2.len;
		/// copy and append s2 to the end of s1 directly if s1 is the end
		/// and the space is sufficient
		if (isatend(s1, s2.len)) {
			text.str = s1.str;
			memcpy(alloc(s2.len), s2.str, s2.len);
		} else {
			char *p;
			text.str = p = alloc(s1.len + s2.len);
			memcpy(p,          s1.str, s1.len);
			memcpy(p + s1.len, s2.str, s2.len);
		}
		return text;
	} 
}
T Text_reverse(T s) {
	assert(s.len >= 0 && s.str);
	if (s.len == 0)
		return Text_null;
	else if (s.len == 1)
		return s;
	else {
		T text;
		char *p;
		int i = s.len;
		text.len = s.len;
		text.str = p = alloc(s.len);
		while (--i >= 0)
			*p++ = s.str[i];
		return text;
	}
}
T Text_map(T s, const T *from, const T *to) {
	/// a static array keeps the result to the next call
	static char map[256];
	static int inited = 0;
	assert(s.len >= 0 && s.str);
	if (from && to) {
		int k;
		/// initialization of the map, this happens every time if to and from are both provided
		for (k = 0; k < (int)sizeof map; k++)
			map[k] = k;
		assert(from->len == to->len);
		for (k = 0; k < from->len; k++)
			map[(unsigned char)from->str[k]] = to->str[k];
		inited = 1;
	} else {
		assert(from == NULL && to == NULL);
		assert(inited);
	}
	if (s.len == 0)
		return Text_null;
	/// if one from and to is not provided, then use the map which was used last time
	else {
		T text;
		int i;
		char *p;
		text.len = s.len;
		text.str = p = alloc(s.len);
		for (i = 0; i < s.len; i++)
			*p++ = map[(unsigned char)s.str[i]];
		return text;
	}
}
int Text_cmp(T s1, T s2) {
	assert(s1.len >= 0 && s1.str);
	assert(s2.len >= 0 && s2.str);
	if (s1.str == s2.str)
		return s1.len - s2.len;
	else if (s1.len < s2.len) {
		int cond = memcmp(s1.str, s2.str, s1.len);
		return cond == 0 ? -1 : cond;
	} else if (s1.len > s2.len) {
		int cond = memcmp(s1.str, s2.str, s2.len);
		return cond == 0 ? +1 : cond;
	} else
		return memcmp(s1.str, s2.str, s1.len);
}
/// These two functions are related to memory control
/// whenever one uses the Text interface, he should
/// call Text_save before all usages,
/// and call Text_restore after all works have been finished
/// this designe is similar to va_start and va_end

/// saves a snap shot of a string just made on chunks
Text_save_T Text_save(void) {
	Text_save_T save;
	NEW(save);
	save->current = current;
	save->avail = current->avail;
	alloc(1);
	return save;
}
/// roll back to the snap shot kept in save
void Text_restore(Text_save_T *save) {
	struct chunk *p, *q;
	assert(save && *save);
	current = (*save)->current;
	current->avail = (*save)->avail;
	FREE(*save);
	for (p = current->link; p; p = q) {
		q = p->link;
		FREE(p);
	}
	current->link = NULL;
}
/// find a char in string
int Text_chr(T s, int i, int j, int c) {
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	for ( ; i < j; i++)
		if (s.str[i] == c)
			return i + 1;
	return 0;
}
/// find a char from the right
int Text_rchr(T s, int i, int j, int c) {
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	while (j > i)
		if (s.str[--j] == c)
			return j + 1;
	return 0;
}
/// find any in the set from the left
int Text_upto(T s, int i, int j, T set) {
	assert(set.len >= 0 && set.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	for ( ; i < j; i++)
		if (memchr(set.str, s.str[i], set.len))
			return i + 1;
	return 0;
}
/// find any in the set from the right
int Text_rupto(T s, int i, int j, T set) {
	assert(set.len >= 0 && set.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	while (j > i)
		if (memchr(set.str, s.str[--j], set.len))
			return j + 1;
	return 0;
}
/// find substring
int Text_find(T s, int i, int j, T str) {
	assert(str.len >= 0 && str.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	if (str.len == 0)
		return i + 1;
	else if (str.len == 1) {
		for ( ; i < j; i++)
			if (s.str[i] == *str.str)
				return i + 1;
	} else
		for ( ; i + str.len <= j; i++)
			if (equal(s, i, str))
				return i + 1;
	return 0;
}
int Text_rfind(T s, int i, int j, T str) {
	assert(str.len >= 0 && str.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	if (str.len == 0)
		return j + 1;
	else if (str.len == 1) {
		while (j > i)
			if (s.str[--j] == *str.str)
				return j + 1;
	} else
		for ( ; j - str.len >= i; j--)
			if (equal(s, j - str.len, str))
				return j - str.len + 1;
	return 0;
}
/// similar to upto, but returns i + 2 instead of i + 1
int Text_any(T s, int i, T set) {
	assert(s.len >= 0 && s.str);
	assert(set.len >= 0 && set.str);
	i = idx(i, s.len);
	assert(i >= 0 && i <= s.len);
	if (i < s.len && memchr(set.str, s.str[i], set.len))
		return i + 2;
	return 0;
}
/// find a consecutive string in which all chars are in set
/// and return the index after this sequence
int Text_many(T s, int i, int j, T set) {
	assert(set.len >= 0 && set.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	if (i < j && memchr(set.str, s.str[i], set.len)) {
		do
			i++;
		while (i < j
		&& memchr(set.str, s.str[i], set.len));
		return i + 1;
	}
	return 0;
}
/// find the position before the occurance of the consecutive sequence in set
int Text_rmany(T s, int i, int j, T set) {
	assert(set.len >= 0 && set.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	if (j > i && memchr(set.str, s.str[j-1], set.len)) {
		do
			--j;
		while (j >= i
		&& memchr(set.str, s.str[j], set.len));
		return j + 2;
	}
	return 0;
}
/// similar to find, but returns the position after the occurance
int Text_match(T s, int i, int j, T str) {
	assert(str.len >= 0 && str.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	if (str.len == 0)
		return i + 1;
	else if (str.len == 1) {
		if (i < j && s.str[i] == *str.str)
			return i + 2;
	} else if (i + str.len <= j && equal(s, i, str))
		return i + str.len + 1;
	return 0;
}
/// similar to rfind, but returns the positoin before occurance
int Text_rmatch(T s, int i, int j, T str) {
	assert(str.len >= 0 && str.str);
	assert(s.len >= 0 && s.str);
	i = idx(i, s.len);
	j = idx(j, s.len);
	if (i > j) { int t = i; i = j; j = t; }
	assert(i >= 0 && j <= s.len);
	if (str.len == 0)
		return j + 1;
	else if (str.len == 1) {
		if (j > i && s.str[j-1] == *str.str)
			return j;
	} else if (j - str.len >= i
	&& equal(s, j - str.len, str))
		return j - str.len + 1;
	return 0;
}
void Text_fmt(int code, va_list_box *box,
	int put(int c, void *cl), void *cl,
	unsigned char flags[], int width, int precision) {
	T *s;
	assert(box && flags);
	s = va_arg(box->ap, T*);
	assert(s && s->len >= 0 && s->str);
	Fmt_puts(s->str, s->len, put, cl, flags,
		width, precision);
}
