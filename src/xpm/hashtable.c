/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* hashtable.c:                                                                *
*                                                                             *
*  XPM library                                                                *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
*  this originaly comes from Colas Nahaboo as a part of Wool                  *
*                                                                             *
\*****************************************************************************/

#include "xpmP.h"

LFUNC(AtomMake, xpmHashAtom, (char *name, void *data));
LFUNC(HashTableGrows, int, (xpmHashTable *table));

static xpmHashAtom
AtomMake(name, data)			/* makes an atom */
    char *name;				/* WARNING: is just pointed to */
    void *data;
{
    xpmHashAtom object = (xpmHashAtom) malloc(sizeof(struct _xpmHashAtom));
    if (object) {
	object->name = name;
	object->data = data;
    }
    return object;
}

/************************\
* 			 *
*  hash table routines 	 *
* 			 *
\************************/

/*
 * Hash function definition:
 * HASH_FUNCTION: hash function, hash = hashcode, hp = pointer on char,
 *				 hash2 = temporary for hashcode.
 * INITIAL_TABLE_SIZE in slots
 * HASH_TABLE_GROWS how hash table grows.
 */

/* Mock lisp function */
#define HASH_FUNCTION 	  hash = (hash << 5) - hash + *hp++;
/* #define INITIAL_HASH_SIZE 2017 */
#define INITIAL_HASH_SIZE 256		/* should be enough for colors */
#define HASH_TABLE_GROWS  size = size * 2;

/* aho-sethi-ullman's HPJ (sizes should be primes)*/
#ifdef notdef
#define HASH_FUNCTION	hash <<= 4; hash += *hp++; \
    if(hash2 = hash & 0xf0000000) hash ^= (hash2 >> 24) ^ hash2;
#define INITIAL_HASH_SIZE 4095		/* should be 2^n - 1 */
#define HASH_TABLE_GROWS  size = size << 1 + 1;
#endif

/* GNU emacs function */
/*
#define HASH_FUNCTION 	  hash = (hash << 3) + (hash >> 28) + *hp++;
#define INITIAL_HASH_SIZE 2017
#define HASH_TABLE_GROWS  size = size * 2;
*/

/* end of hash functions */

/*
 * The hash table is used to store atoms via their NAME:
 *
 * NAME --hash--> ATOM |--name--> "foo"
 *		       |--data--> any value which has to be stored
 *
 */

/*
 * xpmHashSlot gives the slot (pointer to xpmHashAtom) of a name
 * (slot points to NULL if it is not defined)
 *
 */

xpmHashAtom *
xpmHashSlot(table, s)
    xpmHashTable *table;
    char *s;
{
    xpmHashAtom *atomTable = table->atomTable;
    unsigned int hash;
    xpmHashAtom *p;
    char *hp = s;
    char *ns;

    hash = 0;
    while (*hp) {			/* computes hash function */
	HASH_FUNCTION
    }
    p = atomTable + hash % table->size;
    while (*p) {
	ns = (*p)->name;
	if (ns[0] == s[0] && strcmp(ns, s) == 0)
	    break;
	p--;
	if (p < atomTable)
	    p = atomTable + table->size - 1;
    }
    return p;
}

static int
HashTableGrows(table)
    xpmHashTable *table;
{
    xpmHashAtom *atomTable = table->atomTable;
    int size = table->size;
    xpmHashAtom *t, *p;
    int i;
    int oldSize = size;

    t = atomTable;
    HASH_TABLE_GROWS
	table->size = size;
    table->limit = size / 3;
    atomTable = (xpmHashAtom *) malloc(size * sizeof(*atomTable));
    if (!atomTable)
	return (XpmNoMemory);
    table->atomTable = atomTable;
    for (p = atomTable + size; p > atomTable;)
	*--p = NULL;
    for (i = 0, p = t; i < oldSize; i++, p++)
	if (*p) {
	    xpmHashAtom *ps = xpmHashSlot(table, (*p)->name);
	    *ps = *p;
	}
    free(t);
    return (XpmSuccess);
}

/*
 * xpmHashIntern(table, name, data)
 * an xpmHashAtom is created if name doesn't exist, with the given data.
 */

int
xpmHashIntern(table, tag, data)
    xpmHashTable *table;
    char *tag;
    void *data;
{
    xpmHashAtom *slot;

    if (!*(slot = xpmHashSlot(table, tag))) {
	/* undefined, make a new atom with the given data */
	if (!(*slot = AtomMake(tag, data)))
	    return (XpmNoMemory);
	if (table->used >= table->limit) {
	    int ErrorStatus;
	    if ((ErrorStatus = HashTableGrows(table)) != XpmSuccess)
		return(ErrorStatus);
	    table->used++;
	    return (XpmSuccess);
	}
	table->used++;
    }
    return (XpmSuccess);
}

/*
 *  must be called before allocating any atom
 */

int
xpmHashTableInit(table)
    xpmHashTable *table;
{
    xpmHashAtom *p;
    xpmHashAtom *atomTable;

    table->size = INITIAL_HASH_SIZE;
    table->limit = table->size / 3;
    table->used = 0;
    atomTable = (xpmHashAtom *) malloc(table->size * sizeof(*atomTable));
    if (!atomTable)
	return (XpmNoMemory);
    for (p = atomTable + table->size; p > atomTable;)
	*--p = NULL;
    table->atomTable = atomTable;
    return (XpmSuccess);
}

/*
 *   frees a hashtable and all the stored atoms
 */

void
xpmHashTableFree(table)
    xpmHashTable *table;
{
    xpmHashAtom *p;
    xpmHashAtom *atomTable = table->atomTable;
    for (p = atomTable + table->size; p > atomTable;)
	if (*--p)
	    free(*p);
    free(atomTable);
    table->atomTable = NULL;
}
