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
 * Test rendering images
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

#define INTERVAL 0.001

typedef struct vec2 {
	int x, y;
} Vec2;

static void tick(void);
static void test_event(unsigned long, void *);
static void test_collision(int, int, void *);
static void delete_on_collision(int, int, void *);

static int main_font;
static int test_event_ctx;

int
main(void)
{
	Gc *gc;
	Image *img;
	int x, y, player;
	Entity e;

	log_add_fd_sink(1, TRACE);
	log_add_fd_sink(2, ERROR);

	LOG_INFO("initialising graphical context");
	gc = gc_new();
	if (gc == NULL) {
		LOG_ERROR("failed at allocating graphical context");
		return 1;
	}
	gc_init(gc);

	memset(&e, 0, sizeof(Entity));
	img = ff_load("assets/tux.ff");
	if (!img) {
		LOG_ERROR("error loading sprite");
		return 1;
	}
	e.sprite = gc_create_sprite(gc, img, 32, 32);
	e.w = e.h = 3800;
	free(img->d);
	free(img);
	e.x = e.y = 30000;
	e.z = 5;
	e.class = PLAYER;
	player = spawn_obj(e);
	set_collsion(player, NPC, test_collision, NULL);
	set_collsion(player, ITEM, delete_on_collision, NULL);

	/* spawn npc */
	e.x = 60000;
	e.y = 20000;
	e.class = NPC;
	spawn_obj(e);

	img = ff_load("assets/sword.ff");
	test_event_ctx = gc_create_sprite(gc, img, 32, 32);
	free(img->d);
	free(img);

	/* load font */
	img = ff_load("assets/ibm10x22.ff");
	if (!img) {
		LOG_ERROR("error loading sprite");
		return 1;
	}
	main_font = gc_create_sprite(gc, img, 10, 22);
	free(img->d);
	free(img);

	img = ff_load("assets/grass.ff");
	if (!img) {
		LOG_ERROR("error loading sprite");
		return 1;
	}
	e.sprite = gc_create_sprite(gc, img, 64, 64);
	e.z = 0;
	e.class = 0;
	free(img->d);
	free(img);
	for (y = 0; y < 16; ++y) {
		for (x = 0; x < 20; ++x) {
			e.x = 6400 * x;
			e.y = 6400 * y;
			spawn_obj(e);
		}
	}

	gc_bind_input(gc);

	schedule(8000, &test_event, &test_event_ctx);

	while (gc_alive(gc)) {
		while (gc_check_timer(INTERVAL))
			tick();
		gc_clear(gc);
		render_objs(gc);
		//gc_draw(gc, main_font, 200, 200, 0, 1, 0);
		gc_print(gc, main_font, 32, 400, 1, "> Hello world!", 0);
		//gc_print(gc, main_font, 32, 32, 1, "I need scissors 61!", 0);
		/*
		gc_print(gc, main_font, 320, 500, 1, "[Yes]  No ", 0);
		gc_print(gc, main_font, 320, 588, 1, "int\nmain(void)\n{\n}", 0);
		*/
		gc_commit(gc);
	}

	return 0;
}

static void
tick(void)
{
	static unsigned long ntick = 0;
	/*
	Input user_input;

	user_input = gc_poll_input();
	if (user_input.dx && user_input.dy) {
		user_input.dx *= .7f;
		user_input.dy *= .7f;
	}
	tux.acc.x = 5.f * user_input.dx - (int)(tux.vel.x * .005f);
	tux.acc.y = 5.f * user_input.dy - (int)(tux.vel.y * .005f);
	tux.vel.x = (tux.vel.x + tux.acc.x) * .9f;
	tux.vel.y = (tux.vel.y + tux.acc.y) * .9f;
	tux.vel.x = 10.f * user_input.dx;
	tux.vel.y = 10.f * user_input.dy;
	tux.pos.x += tux.vel.x;
	tux.pos.y += tux.vel.y;
	*/
	process_objs();
	collision_poll(ntick);
	schedule_poll(++ntick);
}

static void
test_event(unsigned long now, void *ctx)
{
	Entity e;
	//static int lim = 10;

	/*
	if (!--lim)
		return;
	e.sprite = *(int *)ctx;
	e.z = 5;
	e.class = NPC;
	e.x = 30000 + rand() % 100000;
	e.y = 30000 + rand() % 100000;
	e.w = e.h = 3800;
	spawn_obj(e);
	*/
	e.sprite = *(int *)ctx;
	e.z = 5;
	e.class = ITEM;
	e.x = 70000;
	e.y = 20000;
	e.w = e.h = 3800;
	spawn_obj(e);
	LOG_INFO("spawned at %zu tick (%d x %d)", now, e.x, e.y);
	//schedule(now + 1000, &test_event, ctx);
}

static void
test_collision(int first, int second, void *ctx)
{
	LOG_INFO("entity #%d collided with #%d", first, second);
	LOG_INFO("spawned txt %d", spawn_text(400, 850, main_font, "It's dangerous to go alone.\nTake this!", 38));
}

static void
delete_on_collision(int first, int second, void *ctx)
{
	LOG_INFO("removing entity #%d", second);
	wipe_obj(second);
}
