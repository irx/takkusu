/**
 * Copyright (c) 2021-2024 Max Mruszczak <u at one u x dot o r g>
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
 * Schedule timed events
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "ff.h"
#include "render.h"
#include "entity.h"

#define MAX_SCHEDS 256
#define MAX_COLLS 256


static struct {
	int active[MAX_SCHEDS];
	unsigned long tick[MAX_SCHEDS];
	void *ctx[MAX_SCHEDS], (*fn[MAX_SCHEDS])(unsigned long, void *);
	size_t n;
} schedtab;

static struct {
	int active[MAX_COLLS], first_entity[MAX_COLLS], second_entity[MAX_COLLS], recurring[MAX_COLLS];
	EntityManager *emgr[MAX_COLLS];
	void *ctx[MAX_COLLS], (*fn[MAX_COLLS])(int, int, void *);
	size_t n;
} collisiontab;

int
schedule(unsigned long tick, void (*fn)(unsigned long, void *), void *data)
{
	size_t i;

	for (i = 0; i < schedtab.n; ++i) {
		if (!schedtab.active[i])
			break;
	}
	if (i >= MAX_SCHEDS) {
		LOG_ERROR("reached limit of active schedules (%zu)", i);
		return -1;
	}
	if (i == schedtab.n)
		++schedtab.n;
	schedtab.tick[i] = tick;
	schedtab.fn[i] = fn;
	schedtab.ctx[i] = data;
	schedtab.active[i] = 1;
	LOG_TRACE("scheduled event #%zu", i);

	return i;
}

void
schedule_poll(unsigned int tick)
{
	size_t i;

	for (i = 0; i < schedtab.n; ++i) {
		if (schedtab.active[i] && schedtab.tick[i] <= tick) {
			schedtab.fn[i](tick, schedtab.ctx[i]);
			schedtab.active[i] = 0;
		}
	}
}

int
set_collsion(EntityManager *emgr, int first_entity, int second_entity, void (*fn)(int, int, void *), void *data)
{
	size_t i;

	for (i = 0; i < collisiontab.n; ++i) {
		if (!collisiontab.active[i])
			break;
	}
	if (i >= MAX_COLLS) {
		LOG_ERROR("reached limit of active collision events (%zu)", i);
		return -1;
	}
	if (i == collisiontab.n)
		++collisiontab.n;
	collisiontab.first_entity[i] = first_entity;
	collisiontab.second_entity[i] = second_entity;
	collisiontab.emgr[i] = emgr;
	collisiontab.fn[i] = fn;
	collisiontab.ctx[i] = data;
	collisiontab.active[i] = 1;
	LOG_TRACE("added collision event #%zu", i);

	return i;
}

void
collision_poll(unsigned int tick)
{
	size_t i;
	int result;

	for (i = 0; i < collisiontab.n; ++i) {
		if (!collisiontab.active[i])
			continue;
		result = entity_detect_collision(collisiontab.emgr[i], collisiontab.first_entity[i], collisiontab.second_entity[i], collisiontab.fn[i], collisiontab.ctx[i]);
		if (result < 0) {
			LOG_WARNING("entity #%d or #%d does not exist; removing collision event #%zu", collisiontab.first_entity[i], collisiontab.second_entity[i], i);
			collisiontab.active[i] = 0;
		}
		if (result && !collisiontab.recurring[i])
			collisiontab.active[i] = 0;
	}
}
