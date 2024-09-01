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
#include "audio.h"
#include "entity.h"
//#include "sched.h"

#define ABS(x) ((x < 0) ? -x : x)
#define MAX_ENTITIES 1024

#define ANIM_TICKS_PER_FRAME 150
#define ANIM_TEXT_TICKS_PER_FRAME ANIM_TICKS_PER_FRAME/2
#define ANIM_TEXT_CHARS_PER_FRAME 3
#define ANIM_TEXT_SOUND "blip"
#define ANIM_MAX_FRAMES 4

enum anim_fields {
	ANIM_FRAME = 0,
	ANIM_DIR,
	ANIM_TICKS,
	ANIM_NFIELDS
};

enum anim_direction {
	ANIM_DIR_DOWN = 0,
	ANIM_DIR_UP,
	ANIM_DIR_LEFT,
	ANIM_DIR_RIGHT
};

typedef struct vec2 {
	int x, y;
} Vec2;

typedef struct text {
	const char *str;
	size_t len;
	int font;
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
	size_t anim[MAX_ENTITIES][ANIM_NFIELDS];
} Components;

struct entity_manager {
	Entities entities;
	Components components;
};

static size_t entity_get_subset(const EntityManager *, int *, size_t, uint32_t);
/* Entity `Systems' functions declarations */
static void entity_render(EntityManager *, Gc *, int *, size_t);
static void entity_render_texts(EntityManager *, Gc *, int *, size_t);
static void entity_accelerate(GameState *, int *, size_t);
static void entity_displace(GameState *, int *, size_t);
static void entity_animate_vel(GameState *, int *, size_t);
static void entity_animate_text(GameState *, int *, size_t);

#define NSYSTEMS 4
#define NRENDERSYSTEMS 2

/* Entity `Systems' vtable */
static const struct {
	uint32_t mask; /* system signature; a mask for filtering entity list before
	                  passing it over to the `system' function */
	void (*fn)(GameState *, int *, size_t); /* `system' function ptr */
} systems_vtable[NSYSTEMS] = {
	{
		/* move controllable objects (e.g. player) */
		.mask = (COMPONENT_ACC | COMPONENT_INPUT),
		.fn = entity_accelerate
	},
	{
		/* move rigid bodies */
		.mask = (COMPONENT_ACC | COMPONENT_VEL | COMPONENT_POS | COMPONENT_DIM),
		.fn = entity_displace
	},
	{
		/* animate moving objects */
		.mask = (COMPONENT_VEL | COMPONENT_SPRITE | COMPONENT_ANIM),
		.fn = entity_animate_vel
	},
	{
		/* animate text */
		.mask = (COMPONENT_TEXT | COMPONENT_ANIM),
		.fn = entity_animate_text
	}
};

static const struct {
	uint32_t mask;
	void (*fn)(EntityManager *, Gc *, int *, size_t);
} render_systems_vtable[NRENDERSYSTEMS] = {
	{
		/* render sprites */
		.mask = (COMPONENT_SPRITE | COMPONENT_DIM | COMPONENT_POS | COMPONENT_ZPOS),
		.fn = entity_render
	},
	{
		/* print texts */
		.mask = (COMPONENT_TEXT | COMPONENT_POS),
		.fn = entity_render_texts
	}
};


EntityManager *
create_entity_manager(void)
{
	EntityManager *emgr;
	size_t z;

	z = sizeof(EntityManager);
	emgr = calloc(z, 1);
	if (!emgr)
		LOG_FATAL("failed allocating a new entity manager");
	LOG_TRACE("allocated %zuB for an entity manager", z);

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
	emgr->components.anim[i][ANIM_FRAME] =
	emgr->components.anim[i][ANIM_DIR] =
	emgr->components.anim[i][ANIM_TICKS] = 0;

	return i;
}

int
entity_spawn_text(EntityManager *emgr, int font, int x, int y, const char *str, int animate)
{
	int id;
	EntityInfo info;

	info.components = (COMPONENT_POS | COMPONENT_TEXT);
	info.x = x;
	info.y = y;
	id = entity_spawn(emgr, info);
	if (id < 0)
		return id;

	if (animate) {
		emgr->entities.components[id] |= COMPONENT_ANIM;
		emgr->components.text[id].len = 1;
		emgr->components.anim[id][0] = 0;
	} else {
		emgr->components.text[id].len = strlen(str);
	}
	emgr->components.text[id].str = str;
	emgr->components.text[id].font = font;

	return id;
}

int
entity_get_info(EntityManager *emgr, int id, EntityInfo *e)
{
	if (id >= MAX_ENTITIES || !emgr->entities.exists[id]) {
		LOG_ERROR("cannot get info for non-existent entity #%d", id);
		return 0;
	}
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
	if (id >= MAX_ENTITIES || !emgr->entities.exists[id]) {
		LOG_WARNING("cannot delete non-existent entity #%d", id);
		return;
	}
	emgr->entities.exists[id] = 0;
	if (emgr->entities.n == id + 1)
		--emgr->entities.n;
	LOG_TRACE("removed entity #%d", id);
}

/**
 * Get a subset of entites that fit signature
 * Entity id's are stored into the provided buffer and the amount of found
 * entities is returned
 */
static size_t
entity_get_subset(const EntityManager *emgr, int *buf, size_t len, uint32_t mask)
{
	size_t i, n;

	n = 0;
	for (i = 0; i < emgr->entities.n; ++i) {
		if (n+1 > len)
			LOG_FATAL("entity id buffer exceeded");
		if (emgr->entities.exists[i] && (emgr->entities.components[i] & mask) == mask)
			buf[n++] = i;
	}

	return n;
}

void
process_tick(GameState *state)
{
	int i, ids[MAX_ENTITIES];
	size_t cnt;

	for (i = 0; i < NSYSTEMS; ++i) {
		cnt = entity_get_subset(state->entity_manager, &ids[0], MAX_ENTITIES, systems_vtable[i].mask);
		systems_vtable[i].fn(state, ids, cnt);
	}
}

void
process_rendering(GameState *state)
{
	int i, ids[MAX_ENTITIES];
	size_t cnt;

	for (i = 0; i < NRENDERSYSTEMS; ++i) {
		cnt = entity_get_subset(state->entity_manager, &ids[0], MAX_ENTITIES, render_systems_vtable[i].mask);
		render_systems_vtable[i].fn(state->entity_manager, state->gc, ids, cnt);
	}
}

static void
entity_render(EntityManager *emgr, Gc *gc, int *ids, size_t cnt)
{
	int i, nbg, nfg, bg[MAX_ENTITIES], fg[MAX_ENTITIES];

	/* divide elements into foreground and background groups based on zpos */
	/* TODO use zbuffer capabilities in renderer instead */
	nbg = nfg = 0;
	for (i = 0; i < cnt; ++i)
		if (emgr->components.zpos[ids[i]]) {
			fg[nfg++] = ids[i];
		} else {
			bg[nbg++] = ids[i];
		}

	while (nbg--)
		gc_draw(gc,
			emgr->components.sprite[bg[nbg]].id,
			emgr->components.pos[bg[nbg]].x/100,
			emgr->components.pos[bg[nbg]].y/100,
			emgr->components.zpos[bg[nbg]],
			emgr->components.sprite[bg[nbg]].offs_x,
			emgr->components.sprite[bg[nbg]].offs_y);
	while (nfg--)
		gc_draw(gc,
			emgr->components.sprite[fg[nfg]].id,
			emgr->components.pos[fg[nfg]].x/100,
			emgr->components.pos[fg[nfg]].y/100,
			emgr->components.zpos[fg[nfg]],
			emgr->components.sprite[fg[nfg]].offs_x,
			emgr->components.sprite[fg[nfg]].offs_y);
}

static void
entity_render_texts(EntityManager *emgr, Gc *gc, int *ids, size_t cnt)
{
	int i, id;

	for (i = 0; i < cnt; ++i) {
		id = ids[i];
		gc_print(gc,
			emgr->components.text[id].font,
			emgr->components.pos[id].x / 100,
			emgr->components.pos[id].y / 100,
			5, /* TODO zpos rework */
			emgr->components.text[id].str,
			emgr->components.text[id].len);
	}
}

static void
entity_accelerate(GameState *state, int *ids, size_t cnt)
{
	int i, id;
	Input user_input;
	EntityManager *emgr;

	emgr = state->entity_manager;
	user_input = gc_poll_input();
	if (user_input.dx && user_input.dy) {
		user_input.dx *= .7f;
		user_input.dy *= .7f;
	}
	for (i = 0; i < cnt; ++i) {
		id = ids[i];
		emgr->components.acc[id].x = 5.f * user_input.dx - (int)(emgr->components.vel[id].x * .005f);
		emgr->components.acc[id].y = 5.f * user_input.dy - (int)(emgr->components.vel[id].y * .005f);
	}
}

static void
entity_displace(GameState *state, int *ids, size_t cnt)
{
	int i, id;
	EntityManager *emgr;

	emgr = state->entity_manager;
	for (i = 0; i < cnt; ++i) {
		id = ids[i];
		emgr->components.vel[id].x = (emgr->components.vel[id].x + emgr->components.acc[id].x) * .9f;
		emgr->components.vel[id].y = (emgr->components.vel[id].y + emgr->components.acc[id].y) * .9f;
		emgr->components.pos[id].x += emgr->components.vel[id].x;
		emgr->components.pos[id].y += emgr->components.vel[id].y;
	}
}

static void
entity_animate_vel(GameState *state, int *ids, size_t cnt)
{
	int i, id;
	Vec2 vel;
	EntityManager *emgr;

	emgr = state->entity_manager;
	for (i = 0; i < cnt; ++i) {
		id = ids[i];
		vel = emgr->components.vel[id];
		if (vel.x == 0 && vel.y == 0) {
			emgr->components.anim[id][ANIM_FRAME] = 0;
			continue;
		}
		/* eval direction */
		if (ABS(vel.x) > ABS(vel.y)) {
			if (vel.x > 0)
				emgr->components.anim[id][ANIM_DIR] = ANIM_DIR_RIGHT;
			else
				emgr->components.anim[id][ANIM_DIR] = ANIM_DIR_LEFT;
		} else {
			if (vel.y > 0)
				emgr->components.anim[id][ANIM_DIR] = ANIM_DIR_DOWN;
			else
				emgr->components.anim[id][ANIM_DIR] = ANIM_DIR_UP;
		}
		/* eval frame */
		if (++emgr->components.anim[id][ANIM_TICKS] > ANIM_TICKS_PER_FRAME) {
			emgr->components.anim[id][ANIM_TICKS] = 0;
			emgr->components.anim[id][ANIM_FRAME] = (emgr->components.anim[id][ANIM_FRAME] + 1) % ANIM_MAX_FRAMES;
		}
	}

	/* apply offset to the sprite */
	for (i = 0; i < cnt; ++i) {
		id = ids[i];
		emgr->components.sprite[id].offs_x = emgr->components.anim[id][ANIM_FRAME];
		emgr->components.sprite[id].offs_y = emgr->components.anim[id][ANIM_DIR];
	}
}

static void
entity_animate_text(GameState *state, int *ids, size_t cnt)
{
	int i, id;
	size_t slen;
	Text *txt;
	EntityManager *emgr;

	emgr = state->entity_manager;
	for (i = 0; i < cnt; ++i) {
		id = ids[i];
		txt = &emgr->components.text[id];

		if (++emgr->components.anim[id][0] <= ANIM_TEXT_TICKS_PER_FRAME)
			continue;

		emgr->components.anim[id][0] = 0;
		txt->len += ANIM_TEXT_CHARS_PER_FRAME;
		audio_play(state->audio, ANIM_TEXT_SOUND, 1.f);

		slen = strlen(txt->str);
		if (txt->len >= slen) { /* animation is finished */
			txt->len = slen;
			emgr->entities.components[id] ^= COMPONENT_ANIM;
			LOG_TRACE("finished animating text #%d", id);
		}
	}
}

/**
 * A temporary function for backwards compat with previous entity system
 * TODO replace with a general purpose collision system
 */
int
entity_detect_collision(EntityManager *emgr, int first, int second, void (*fn)(int, int, void *), void *ctx)
{
	uint32_t mask;
	Vec2 first_pos, first_dim, second_pos, second_dim;

	/* check if entities are valid */
	if (!emgr->entities.exists[first] || !emgr->entities.exists[second])
		return -1;
	mask = (COMPONENT_POS | COMPONENT_DIM);
	if ((emgr->entities.components[first] & mask) != mask
		|| (emgr->entities.components[second] & mask) != mask)
		return -1;

	first_pos = emgr->components.pos[first];
	second_pos = emgr->components.pos[second];
	first_dim = emgr->components.dim[first];
	second_dim = emgr->components.dim[second];

	if (first_pos.x < second_pos.x + second_dim.x &&
		first_pos.x + first_dim.x > second_pos.x &&
		first_pos.y < second_pos.y + second_dim.y &&
		first_pos.y + first_dim.x > second_pos.y) {
		if (fn)
			fn(first, second, ctx);
		return 1;
	}

	return 0;
}
