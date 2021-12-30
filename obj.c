/**
 * Copyright (c) 2021 Max Mruszczak <u at one u x dot o r g>
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
 * Manage objects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "log.h"
#include "ff.h"
#include "render.h"
#include "obj.h"
#include "sched.h"

#define ABS(x) ((x < 0) ? -x : x)
#define MAX_OBJ 1024
#define ANIM_TICS_PER_FRAME 150

typedef struct vec2 {
	int x, y;
} Vec2;

static struct {
	Vec2 area[MAX_OBJ], pos[MAX_OBJ], vel[MAX_OBJ], acc[MAX_OBJ];
	int sprite[MAX_OBJ], zpos[MAX_OBJ], occup[MAX_OBJ];
	enum class class[MAX_OBJ];
	size_t anim[MAX_OBJ][3], n;
} objects;

static struct {
	Vec2 pos[MAX_OBJ];
	int font[MAX_OBJ], occup[MAX_OBJ];
	char *str[MAX_OBJ];
	size_t anim[MAX_OBJ], len[MAX_OBJ], n;
} texts;

static void anim_text_event(unsigned long, void *);


int
spawn_obj(Entity e)
{
	int i = -1;

	while (objects.occup[++i])
		if (i+1 >= MAX_OBJ) {
			LOG_ERROR("reached limit of objects");
			return -1;
		}
	if (objects.n == i)
		++objects.n;
	objects.occup[i] = 1;
	objects.vel[i].x = 0;
	objects.vel[i].y = 0;
	objects.acc[i] = objects.vel[i];
	objects.sprite[i] = e.sprite;
	objects.pos[i].x = e.x;
	objects.pos[i].y = e.y;
	objects.area[i].x = e.w;
	objects.area[i].y = e.h;
	objects.zpos[i] = e.z;
	objects.class[i] = e.class;
	objects.anim[i][0] = 0;
	objects.anim[i][1] = 0;
	objects.anim[i][2] = 0;

	return i;
}

int
get_obj(int id, Entity *e)
{
	if (id > MAX_OBJ || !objects.occup[id])
		return 0;
	e->sprite = objects.sprite[id];
	e->x = objects.pos[id].x;
	e->y = objects.pos[id].y;
	e->z = objects.zpos[id];
	e->w = objects.area[id].x;
	e->h = objects.area[id].x;
	e->class = objects.class[id];

	return 1;
}

void
wipe_obj(int id)
{
	objects.occup[id] = 0;
	//--objects.n;
}

void
render_objs(Gc *gc)
{
	int i, nbg, nfg, bg[MAX_OBJ], fg[MAX_OBJ];

	nbg = nfg = 0;
	for (i = 0; i < objects.n; ++i)
		if (objects.occup[i]) {
			if (objects.zpos[i]) {
				fg[nfg++] = i;
			} else {
				bg[nbg++] = i;
			}
		}
	while (nbg--)
		gc_draw(gc, objects.sprite[bg[nbg]], objects.pos[bg[nbg]].x/100, objects.pos[bg[nbg]].y/100, objects.zpos[bg[nbg]], 0, 0);
	while (nfg--)
		gc_draw(gc, objects.sprite[fg[nfg]], objects.pos[fg[nfg]].x/100, objects.pos[fg[nfg]].y/100, objects.zpos[fg[nfg]], objects.anim[fg[nfg]][0], objects.anim[fg[nfg]][1]);
	for (i = 0; i < texts.n; ++i) {
		if (texts.occup[i])
			gc_print(gc, texts.font[i], texts.pos[i].x, texts.pos[i].y, 5, texts.str[i], texts.anim[i]);
	}
}

int
spawn_text(int x, int y, int font, char *strptr, size_t len)
{
	int i = -1;

	while (texts.occup[++i])
		if (i+1 >= MAX_OBJ) {
			LOG_ERROR("reached limit of text objects");
			return -1;
		}
	if (texts.n == i)
		++texts.n;
	texts.occup[i] = 1;
	texts.font[i] = font;
	texts.pos[i].x = x;
	texts.pos[i].y = y;
	texts.len[i] = len;
	texts.str[i] = strptr;
	texts.anim[i] = 0;
	schedule(0, &anim_text_event, &texts.occup[i]);

	return i;
}

void
wipe_text(int id)
{
	texts.occup[id] = 0;
	//--objects.n;
}

void
process_objs(void)
{
	Input user_input;
	int i;

	for (i = 0; i < objects.n; ++i) {
		if (!objects.occup[i])
			continue;
		if (i == 0) {
			user_input = gc_poll_input();
			if (user_input.dx && user_input.dy) {
				user_input.dx *= .7f;
				user_input.dy *= .7f;
			}
			/* animation */
			if (ABS(user_input.dx) + ABS(user_input.dy) > 0) {
				if (ABS(user_input.dx) > ABS(user_input.dy)) {
					if (user_input.dx > 0)
						objects.anim[i][1] = 3;
					else
						objects.anim[i][1] = 2;
				} else {
					if (user_input.dy > 0)
						objects.anim[i][1] = 0;
					else
						objects.anim[i][1] = 1;
				}
				if (++objects.anim[i][2] > ANIM_TICS_PER_FRAME) {
					objects.anim[i][2] = 0;
					if (++objects.anim[i][0] > 3)
						objects.anim[i][0] = 0;
				}
			} else
				objects.anim[i][0] = 0;
		} else
			user_input.dx = user_input.dy = 0;
		objects.acc[i].x = 5.f * user_input.dx - (int)(objects.vel[i].x * .005f);
		objects.acc[i].y = 5.f * user_input.dy - (int)(objects.vel[i].y * .005f);
		objects.vel[i].x = (objects.vel[i].x + objects.acc[i].x) * .9f;
		objects.vel[i].y = (objects.vel[i].y + objects.acc[i].y) * .9f;
		/*
		objects.vel[i].x = 10.f * user_input.dx;
		objects.vel[i].y = 10.f * user_input.dy;
		*/
		objects.pos[i].x += objects.vel[i].x;
		objects.pos[i].y += objects.vel[i].y;
	}
}

int
detect_collision(int first, enum class secondclass, void (*fn)(int, int, void *), void *ctx)
{
	int i, n;
	Vec2 firstpos, firstarea;

	if ((size_t)first > objects.n || !objects.occup[first])
		return -1;
	firstpos = objects.pos[first];
	firstarea = objects.area[first];
	n = 0;
	for (i = 0; i < objects.n; ++i) {
		if (i == first || !objects.occup[i] || (objects.class[i] & secondclass) == 0)
			continue;
		if (firstpos.x < objects.pos[i].x + objects.area[i].x &&
		    firstpos.x + firstarea.x > objects.pos[i].x &&
		    firstpos.y < objects.pos[i].y + objects.area[i].y &&
		    firstpos.y + firstarea.x > objects.pos[i].y) {
			++n;
			if (fn)
				fn(first, i, ctx);
		}
	}

	return n;
}

static void
anim_text_event(unsigned long now, void *ctx)
{
	size_t idx;

	idx = (int *)ctx - &texts.occup[0];
	if (texts.occup[idx] && texts.anim[idx] < texts.len[idx]) {
		++texts.anim[idx];
		schedule(now + 100, &anim_text_event, ctx);
	}
}
