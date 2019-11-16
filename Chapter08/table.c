static char rcsid[] = "$Id: table.c 6 2007-01-22 00:45:22Z drhanson $";
#include <limits.h>
#include <stddef.h>
#include "mem.h"
#include "assert.h"
#include "table.h"
#define T Table_T
struct T {
	int size; /// size of the hash table, a prime
	int (*cmp)(const void *x, const void *y);
	unsigned (*hash)(const void *key); /// %size is not contained in hash function
	int length;
	/// timestamp imcrements every time an element is added or deleted
	/// used to keep track of whether the table has been changed during a period of time
	unsigned timestamp;
	struct binding {
		struct binding *link;
		const void *key;
		void *value;
	} **buckets; /// an array of struct binding *s
};
/// default compare function
static int cmpatom(const void *x, const void *y) {
	return x != y;
}
/// default hash function
static unsigned hashatom(const void *key) {
	return (unsigned long)key>>2; /// where key is an atom, key is a const char *
	/// >> 2 to align at word boundary
}
T Table_new(int hint,
	int cmp(const void *x, const void *y),
	unsigned hash(const void *key)) {
	T table;
	int i;
	/// primes closest to 2 to the nth
	static int primes[] = { 509, 509, 1021, 2053, 4093,
		8191, 16381, 32771, 65521, INT_MAX };
	assert(hint >= 0);
	for (i = 1; primes[i] < hint; i++)
		;
	table = ALLOC(sizeof (*table) + /// for header
		primes[i-1]*sizeof (table->buckets[0])); /// for hash table
	table->size = primes[i-1]; /// the size is the largest prime in primes which is smaller than hint
	table->cmp  = cmp  ?  cmp : cmpatom;
	table->hash = hash ? hash : hashatom;
	table->buckets = (struct binding **)(table + 1); /// the place next to the table is for buckets
	for (i = 0; i < table->size; i++)
		table->buckets[i] = NULL; /// initialising the buckets items to NULL
	table->length = 0;
	table->timestamp = 0;
	return table;
}
void *Table_get(T table, const void *key) {
	int i;
	struct binding *p;
	assert(table);
	assert(key);
	i = (*table->hash)(key)%table->size;
	for (p = table->buckets[i]; p; p = p->link)
		if ((*table->cmp)(key, p->key) == 0)
			break;
	return p ? p->value : NULL;
}
/// changes the value existing in the table if the cmp function treat the new value's key as the same
/// to avoid this, use if (Table_get(t, k)) Table_put(t, k, v);
/// however, the value could be NULL
void *Table_put(T table, const void *key, void *value) {
	int i;
	struct binding *p;
	void *prev;
	assert(table);
	assert(key);
	/// note that value could be NULL
	i = (*table->hash)(key)%table->size;
	for (p = table->buckets[i]; p; p = p->link)
		if ((*table->cmp)(key, p->key) == 0)
			break;
	if (p == NULL) {
		NEW(p);
		p->key = key;
		p->link = table->buckets[i];
		table->buckets[i] = p;
		table->length++;
		prev = NULL;
	} else
		/// changes the previous value kept there, and return the previous one
		prev = p->value;
	p->value = value;
	table->timestamp++;
	return prev;
}
int Table_length(T table) {
	assert(table);
	return table->length;
}
void Table_map(T table,
	void apply(const void *key, void **value, void *cl),
	void *cl) {
	int i;
	unsigned stamp;
	struct binding *p;
	assert(table);
	assert(apply);
	stamp = table->timestamp;
	for (i = 0; i < table->size; i++)
		for (p = table->buckets[i]; p; p = p->link) {
			apply(p->key, &p->value, cl);
			/// should not change the table when using map
			assert(table->timestamp == stamp);
		}
}
void *Table_remove(T table, const void *key) {
	int i;
	struct binding **pp;
	assert(table);
	assert(key);
	table->timestamp++;
	i = (*table->hash)(key)%table->size;
	for (pp = &table->buckets[i]; *pp; pp = &(*pp)->link) /// double pointer to manipulate a list
		if ((*table->cmp)(key, (*pp)->key) == 0) {
		/* reference: finding a node using a single pointer
			for (p = table->buckets[i]; p; p = p->link)
				if ((*table->cmp)(key, p->key) == 0)
				...
		*/	
			struct binding *p = *pp;
			void *value = p->value;
			*pp = p->link;
			FREE(p);
			table->length--;
			return value;
		}
	return NULL; /// allowed to remove an element which does not exist in the table
}
void **Table_toArray(T table, void *end) {
	int i, j = 0;
	void **array;
	struct binding *p;
	assert(table);
	array = ALLOC((2*table->length + 1)*sizeof (*array));
	for (i = 0; i < table->size; i++)
		for (p = table->buckets[i]; p; p = p->link) {
			/// (void *): remove const
			array[j++] = (void *)p->key; /// delete this line will make a toKeysArray
			array[j++] = p->value;       /// delete this line will make a toValuesArray
		}
	array[j] = end; /// en end indicator is needed as an array lost its length when returned
	return array;
}
void Table_free(T *table) {
	assert(table && *table);
	if ((*table)->length > 0) {
		int i;
		struct binding *p, *q;
		for (i = 0; i < (*table)->size; i++)
			for (p = (*table)->buckets[i]; p; p = q) {
				q = p->link;
				FREE(p);
			}
	}
	FREE(*table);
}
