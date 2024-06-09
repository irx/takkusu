/**
 * Copyright (c) 2024 Max Mruszczak <u at one u x dot o r g>
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
 * Manage entities
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "log.h"
#include "ff.h"
#include "render.h"
#include "entity.h"
//#include "sched.h"

#define ABS(x) ((x < 0) ? -x : x)
#define MAX_OBJ 1024
#define MAX_ENTITIES 1024
#define ANIM_TICS_PER_FRAME 150

typedef struct vec2 {
	int x, y;
} Vec2;

typedef struct text {
	const char *str;
	size_t len;
} Text;

typedef struct sprite {
	int id;
	int offs_x, offs_y;
} Sprite;

typedef struct entities {
	int exists[MAX_ENTITIES];
	uint32_t components[MAX_ENTITIES];
	size_t n;
} Entities;

typedef struct components {
	Vec2 dim[MAX_ENTITIES], pos[MAX_ENTITIES], vel[MAX_ENTITIES], acc[MAX_ENTITIES];
	int zpos[MAX_ENTITIES];
	Sprite sprite[MAX_ENTITIES];
	Text text[MAX_ENTITIES];
	size_t anim[MAX_ENTITIES][3];
} Components;

struct entity_manager {
	Entities entities;
	Components components;
};

static size_t entity_get_subset(const EntityManager *, int *, size_t, uint32_t);
static void entity_render(EntityManager *, Gc *, int *, size_t);


EntityManager *
create_entity_manager(void)
{
	EntityManager *emgr;
	size_t z;

	z = sizeof(EntityManager);
	emgr = malloc(z);
	if (!emgr)
		LOG_FATAL("failed allocating new entity manager");
	memset(emgr, 0, z);

	return emgr;
}

void
destroy_entity_manager(EntityManager *emgr)
{
	free(emgr);
}

int
entity_spawn(EntityManager *emgr, EntityInfo e)
{
	int i = -1;

	while (emgr->entities.exists[++i])
		if (i+1 >= MAX_ENTITIES) {
			LOG_ERROR("reached limit of entities");
			return -1;
		}
	if (emgr->entities.n == i)
		++emgr->entities.n;
	emgr->entities.exists[i] = 1;
	emgr->entities.components[i] = e.components;
	/* TODO set component data only if the enum is set */
	emgr->components.vel[i].x = 0;
	emgr->components.vel[i].y = 0;
	emgr->components.acc[i] = emgr->components.vel[i];
	emgr->components.sprite[i].id = e.sprite;
	emgr->components.sprite[i].offs_x = 0;
	emgr->components.sprite[i].offs_y = 0;
	emgr->components.pos[i].x = e.x;
	emgr->components.pos[i].y = e.y;
	emgr->components.dim[i].x = e.w;
	emgr->components.dim[i].y = e.h;
	emgr->components.zpos[i] = e.z;
	emgr->components.anim[i][0] = 0;
	emgr->components.anim[i][1] = 0;
	emgr->components.anim[i][2] = 0;

	return i;
}

int
entity_get_info(EntityManager *emgr, int id, EntityInfo *e)
{
	if (id > MAX_ENTITIES || !emgr->entities.exists[id])
		return 0;
	e->sprite = emgr->components.sprite[id].id;
	e->x = emgr->components.pos[id].x;
	e->y = emgr->components.pos[id].y;
	e->z = emgr->components.zpos[id];
	e->w = emgr->components.dim[id].x;
	e->h = emgr->components.dim[id].x;

	return 1;
}

void
entity_delete(EntityManager *emgr, int id)
{
	emgr->entities.exists[id] = 0;
}

/**
 * Get a subset of entites that fit signature
 * Entity id's are stored into the provided buffer and the amount of found
 * entities is returned
 */
static size_t
entity_get_subset(const EntityManager *emgr, int *buf, size_t len, uint32_t sign)
{
	size_t i, n;

	n = 0;
	for (i = 0; i < emgr->entities.n; ++i) {
		if (n+1 > len)
			LOG_FATAL("entity id buffer exceeded");
		if ((emgr->entities.components[i] & sign) == sign)
			buf[n++] = i;
	}

	return n;
}

void
process_tick(GameState *state)
{
	uint32_t sign;
	int ids[MAX_ENTITIES];
	size_t cnt;

	sign = (COMPONENT_SPRITE | COMPONENT_DIM | COMPONENT_POS);
	cnt = entity_get_subset(state->entity_manager, &ids[0], MAX_ENTITIES, sign);
	entity_render(state->entity_manager, state->gc, ids, cnt);
}

static void
entity_render(EntityManager *emgr, Gc *gc, int *ids, size_t cnt)
{
	/*
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
	*/
}
