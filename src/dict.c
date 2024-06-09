/**
 * Copyright (c) 2022-2024 Max Mruszczak <u at one u x dot o r g>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * A general purpose hash map
 * nothing fancy
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "dict.h"

#define MUL 2

typedef struct bucket Bucket;
struct bucket {
	Bucket *prev, *next;
	char *s;
	void *p;
	size_t h;
};

struct dict {
	size_t siz, n;
	Bucket **tab, *first, *last;
}; /* type Dict */

static unsigned int hash(const char *, unsigned int);


/**
 * Create a hashmap with a table of size `siz'
 */
Dict *
dict_create(size_t siz)
{
	Dict *d;

	d = calloc(sizeof(Dict), 1);
	if (!d) {
		LOG_PERROR("failed to allocate dict");
		return NULL;
	}
	d->tab = calloc(sizeof(Bucket *), siz);
	if (!d) {
		LOG_PERROR("failed to allocate bucket table");
		free(d);
		return NULL;
	}
	d->siz = siz;

	return d;
}

void
dict_destroy(Dict *d)
{
	Bucket *b;

	while((b = d->first)) {
		d->first = b->next;
		free(b->s);
		free(b);
	}
	free(d->tab);
	free(d);
}

/**
 * Get value for key `s'
 */
void *
dict_lookup(Dict *d, const char *s)
{
	size_t h;
	Bucket *b;

	h = hash(s, d->siz);
	b = d->tab[h];
	while (b && b->h == h) {
		if (!strcmp(b->s, s))
			return b->p;
		b = b->next;
	}

	return NULL;
}

/**
 * Insert value `p' into a bucket of key `s'
 */
int
dict_put(Dict *d, const char *s, void *p)
{
	size_t h;
	Bucket *b;

	h = hash(s, d->siz);
	LOG_TRACE("putting %s in index %lu", s, h);
	b = d->tab[h];
	/* cycle through linked list buckets */
	while (b && b->h == h) {
		if (!strcmp(b->s, s)) {
			if (p && !b->p)
				++d->n; /* prev value was null (counts as 0 size), new value is not null (counts as 1 size) */
			else if (!p && b->p)
				--d->n; /* reduce apparent size because stored value is null */
			b->p = p;
			return 1; /* exact entry already exists; replace it */
		}
		b = b->next;
	}
	/* create a new bucket */
	b = calloc(sizeof(Bucket), 1);
	if (!b)
		goto putallocfail;
	b->s = strdup(s);
	if (!b->s) {
		free(b);
		goto putallocfail;
	}
	b->p = p;
	b->h = h;
	if (p) /* value has to be non-null in order to be of size 1 as opposed to 0 */
		++d->n;
	if (d->tab[h]) {
		b->next = d->tab[h]->next;
		if (b->next)
			b->next->prev = b;
		d->tab[h]->next = b;
		b->prev = d->tab[h];
		if (b->prev == d->last)
			d->last = b;
		return 1; /* at least one entry with same hash exists; append to the list */
	}
	d->tab[h] = b; /* no entry with the same hash so far */
	if (!d->first) {
		d->first = d->last = b;
		b->next = b->prev = NULL;
		return 1; /* list was empty */
	}
	d->last->next = b;
	b->prev = d->last;
	b->next = NULL;
	d->last = b;
	return 1;

putallocfail:
	LOG_PERROR("failed to allocate new bucket");
	return 0;
}

/**
 * Remove all buckets with null value
 */
size_t
dict_prune(Dict *d)
{
	size_t nfreed;
	Bucket *b, *iter;

	nfreed = 0;
	iter = d->first;
	while (iter) {
		b = iter;
		iter = b->next;
		 /* if value of the bucket is null it shall be removed from the list
		  * and deallocated */
		if (!b->p) {
			if (d->tab[b->h] == b)
				d->tab[b->h] = (b->next && b->next->h == b->h) ? b->next : NULL;
			if (d->first == b)
				d->first = b->next;
			if (d->last == b)
				d->last = b->prev;
			if (b->prev)
				b->prev->next = b->next;
			if (b->next)
				b->next->prev = b->prev;
			free(b->s);
			free(b);
			++nfreed;
		}
	}
	LOG_TRACE("dict prune: freed %lu nodes", nfreed);

	return nfreed;
}

size_t
dict_size(Dict *d)
{
	return d->n;
}

static unsigned int
hash(const char *s, unsigned int nhash)
{
	unsigned int h;
	unsigned char *p;

	h = 0;
	for (p = (unsigned char *)s; *p != '\0'; ++p)
		h = MUL * h + *p;

	return h % nhash;
}
