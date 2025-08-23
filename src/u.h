/**
 * Copyright (c) 2025 Max Mruszczak <u at one u x dot o r g>
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
 * Core typedefs
 */

#include <stdint.h>
#include <stdlib.h>

#define nil ((void *)0)
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef signed char schar;
typedef long long vlong;
typedef unsigned long long uvlong;
typedef unsigned long long uintptr;
typedef size_t usize;
typedef ssize_t ssize;
typedef uint Rune;

typedef uint8_t u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef int8_t s8int;
typedef int16_t s16int;
typedef int32_t s32int;
typedef int64_t s64int;

typedef u8int uint8;
typedef u16int uint16;
typedef u32int uint32;
typedef u64int uint64;
typedef s8int int8;
typedef s16int int16;
typedef s32int int32;
typedef s64int int64;


/* Geometric stuff */

typedef struct vec2 {
	int x, y;
} Vec2;

typedef struct box2 {
	Vec2 pos, dim;
} Box2;

typedef struct bbox2 {
	Vec2 min, max;
} BBox2;

static inline BBox2
to_bbox2(const Box2 *b)
{
	BBox2 bb;
	Vec2 hsiz;

	hsiz.x = b->dim.x / 2;
	hsiz.y = b->dim.y / 2;
	bb.min.x = b->pos.x - hsiz.x;
	bb.max.x = b->pos.x + hsiz.x;
	bb.min.y = b->pos.y - hsiz.y;
	bb.max.y = b->pos.y + hsiz.y;

	return bb;
}

static inline int
bbox2_intersect(const BBox2 *a, const BBox2 *b)
{
	return !(a->min.x > b->max.x || b->min.x > a->max.x
		|| a->min.y > b->max.y || b->min.y > a->max.y);
}

static inline int
box2_intersect(const Box2 *a, const Box2 *b)
{
	return (a->pos.x - a->dim.x / 2 < b->pos.x + b->dim.x / 2
		&& a->pos.x + a->dim.x / 2 > b->pos.x - b->pos.x / 2
		&& a->pos.y - a->dim.y / 2 < b->pos.y + b->dim.y / 2
		&& a->pos.y + a->dim.y / 2 > b->pos.y - b->pos.y / 2);
}


/* Bitset */

/**
 * By default bitset data is stored as a word (uint16) array
 * Convert it to an octet array before to a file etc
 */
typedef union bitset256 {
	uint8 reg8[32];
	uint16 reg16[16];
	uint32 reg32[8];
} Bitset256;

static inline int
bs256_get(const Bitset256 *bs, usize n)
{
	return (bs->reg16[n >> 4] >> (n & 0x0f)) & 1;
}

static inline int
bs256_set(Bitset256 *bs, usize n)
{
	return bs->reg16[n >> 4] |= 1 << (n & 0x0f);
}

static inline int
bs256_clr(Bitset256 *bs, usize n)
{
	return bs->reg16[n >> 4] ^= 1 << (n & 0x0f);
}

/* check if `b' is a subset of `a' */
static inline int
bs256_subset(const Bitset256 *a, const Bitset256 *b)
{
	int i;
	for (i = 0; i < 8; ++i)
		if ((a->reg32[i] & b->reg32[i]) != b->reg32[i])
			return 0;
	return 1;
}

/* convert bitset storage from word array to octet array */
static inline void
bs256_marshall(Bitset256 *bs)
{
	int i, j;
	uint16 w;

	for (i = j = 0; i < 16; ++i) {
		w = bs->reg16[i];
		bs->reg8[j++] = w >> 8;
		bs->reg8[j++] = w & 0xff;
	}
}

/* convert bitset storage from octet array to word array */
static inline void
bs256_unmarshall(Bitset256 *bs)
{
	int i, j;
	uint16 w;

	for (i = j = 0; i < 16; ++i) {
		w = bs->reg8[j++] << 8;
		w |= bs->reg8[j++];
		bs->reg16[i] = w;
	}
}
