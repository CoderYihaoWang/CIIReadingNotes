static char rcsid[] = "$Id: bit.c 6 2007-01-22 00:45:22Z drhanson $";
#include <stdarg.h>
#include <string.h>
#include "assert.h"
#include "bit.h"
#include "mem.h"
#define T Bit_T
struct T {
	int length; /// number of bits
	unsigned char *bytes; /// points to the memory section allocated for the bitset
	unsigned long *words; /// a larger pointer so that comparison is more convenient
};
#define BPW (8*sizeof (unsigned long)) /// 8 * 64 = 512 bits at a time, same as the size of unsigned long
/// ?? ??
#define nwords(len) ((((len) + BPW - 1)&(~(BPW-1)))/BPW) /// how many words are needed to place the bitset
#define nbytes(len) ((((len) + 8 - 1)&(~(8-1)))/8) /// how many bytes are needed
/// sequal: returning value when s == t
/// snull: returning value when s == NULL
/// tnull: returning value when t == NULL
/// op: when none of the above is the case, use op to get the result
#define setop(sequal, snull, tnull, op) \
	if (s == t) { assert(s); return sequal; } \
	else if (s == NULL) { assert(t); return snull; } \
	else if (t == NULL) return tnull; \
	else { \
		int i; T set; \
		assert(s->length == t->length); \
		set = Bit_new(s->length); \
		for (i = nwords(s->length); --i >= 0; ) \
			set->words[i] = s->words[i] op t->words[i]; \
		return set; }
unsigned char msbmask[] = { /// corresponding to lo % 8
	0xFF, 0xFE, 0xFC, 0xF8, /// 11111111 11111110 11101100 11111000
	0xF0, 0xE0, 0xC0, 0x80  /// 11110000 11100000 11000000 10000000
};
unsigned char lsbmask[] = { /// corresponding to hi % 8
	0x01, 0x03, 0x07, 0x0F, /// 00000001 00000011 00000111 00001111
	0x1F, 0x3F, 0x7F, 0xFF  /// 00011111 00111111 01111111 11111111
};
static T copy(T t) {
	T set;
	assert(t);
	set = Bit_new(t->length);
	if (t->length > 0)
		memcpy(set->bytes, t->bytes, nbytes(t->length));
	return set;
}
T Bit_new(int length) {
	T set;
	assert(length >= 0);
	NEW(set);
	if (length > 0)
		set->words = CALLOC(nwords(length),
			sizeof (unsigned long)); /// initialising to 0
	else
		set->words = NULL; /// empty set
	set->bytes = (unsigned char *)set->words; /// bytes and words are the same address
	set->length = length;
	return set;
}
void Bit_free(T *set) {
	assert(set && *set);
	FREE((*set)->words);
	FREE(*set);
}
int Bit_length(T set) {
	assert(set);
	return set->length;
}
int Bit_count(T set) {
	int length = 0, n;
	static char count[] = {
		0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 }; /// number of 1's in a half-byte
	/// 16 possibilities for half-byte:
	/// 0000 0001 0010 0011 
	/// 0100 0101 0110 0111
	/// 1000 1001 1010 1011
	/// 1100 1101 1110 1111
	assert(set);
	for (n = nbytes(set->length); --n >= 0; ) { /// count byte by byte
		unsigned char c = set->bytes[n];
		/// 0xF: 0000 1111, c & 0xF: get the lower half byte, c >> 4: get the higher half byte
		length += count[c&0xF] + count[c>>4];
	}
	return length;
}
int Bit_get(T set, int n) {
	assert(set);
	assert(0 <= n && n < set->length);
	/// the nth bit is the n % 8 th bit of the n / 8 th byte
	return ((set->bytes[n/8]>>(n%8))&1);
}
int Bit_put(T set, int n, int bit) {
	int prev;
	assert(set);
	assert(bit == 0 || bit == 1);
	assert(0 <= n && n < set->length);
	prev = ((set->bytes[n/8]>>(n%8))&1);
	if (bit == 1)
		set->bytes[n/8] |=   1<<(n%8); /// |= : make it 1
	else
		set->bytes[n/8] &= ~(1<<(n%8)); /// &= ~ : make it 0
	return prev;
}
void Bit_set(T set, int lo, int hi) {
		assert(set);
		assert(0 <= lo && hi < set->length);
		assert(lo <= hi);
	if (lo/8 < hi/8) { /// across bytes
		set->bytes[lo/8] |= msbmask[lo%8]; /// ower end
		{                                  /// middle whole bytes
			int i;
			for (i = lo/8+1; i < hi/8; i++)
				set->bytes[i] = 0xFF;
		}
		set->bytes[hi/8] |= lsbmask[hi%8]; /// higher end
	} else /// in the same byte
		set->bytes[lo/8] |= (msbmask[lo%8]&lsbmask[hi%8]); /// combine two masks
}
void Bit_clear(T set, int lo, int hi) {
		assert(set);
		assert(0 <= lo && hi < set->length);
		assert(lo <= hi);
	if (lo/8 < hi/8) {
		int i;
		set->bytes[lo/8] &= ~msbmask[lo%8];
		for (i = lo/8+1; i < hi/8; i++)
			set->bytes[i] = 0;
		set->bytes[hi/8] &= ~lsbmask[hi%8];
	} else
		set->bytes[lo/8] &= ~(msbmask[lo%8]&lsbmask[hi%8]);
}
void Bit_not(T set, int lo, int hi) {
		assert(set);
		assert(0 <= lo && hi < set->length);
		assert(lo <= hi);
	if (lo/8 < hi/8) {
		int i;
		set->bytes[lo/8] ^= msbmask[lo%8];
		for (i = lo/8+1; i < hi/8; i++)
			set->bytes[i] ^= 0xFF; /// ^= 0xff: bit wise negation => ~=
		set->bytes[hi/8] ^= lsbmask[hi%8];
	} else
		set->bytes[lo/8] ^= (msbmask[lo%8]&lsbmask[hi%8]);
}
void Bit_map(T set,
	void apply(int n, int bit, void *cl), void *cl) {
	int n;
	assert(set);
	/// must operate bit wise in order to make sure apply works on the latest version of the bit
	for (n = 0; n < set->length; n++)
		apply(n, ((set->bytes[n/8]>>(n%8))&1), cl); /// get the position and value of the nth bit
													/// and apply cl on it
}
int Bit_eq(T s, T t) {
	int i;
	assert(s && t);
	assert(s->length == t->length);
	for (i = nwords(s->length); --i >= 0; ) /// faster than byte wise
		if (s->words[i] != t->words[i])
			return 0;
	return 1;
}
int Bit_leq(T s, T t) {
	int i;
	assert(s && t);
	assert(s->length == t->length);
	for (i = nwords(s->length); --i >= 0; )
		if ((s->words[i]&~t->words[i]) != 0) /// &~: in s but not in t
			return 0;
	return 1;
}
int Bit_lt(T s, T t) {
	int i, lt = 0;
	assert(s && t);
	assert(s->length == t->length);
	for (i = nwords(s->length); --i >= 0; )
		if ((s->words[i]&~t->words[i]) != 0) /// in s but not in t
			return 0;
		else if (s->words[i] != t->words[i])
			lt |= 1; /// |= 1: set to 1, able to repeat
	return lt; /// lt === 0 if all words are equal
}
/// union: |, intersect: &, minus: &~, diff: ^
T Bit_union(T s, T t) {
	setop(copy(t), copy(t), copy(s), |) /// no ; because it is expanded from function-like macro
										/// wrapping the macro in a do while and fix this, if needed
}
T Bit_inter(T s, T t) {
	setop(copy(t),
		Bit_new(t->length), Bit_new(s->length), &)
}
T Bit_minus(T s, T t) {
	setop(Bit_new(s->length),
		Bit_new(t->length), copy(s), & ~) /// when s == NULL, s - t is an empty set
}
T Bit_diff(T s, T t) {
	setop(Bit_new(s->length), copy(t), copy(s), ^)
}
