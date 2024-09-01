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
 * Test entity and rendering system
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
#include "sched.h"
#include "dict.h"

#ifdef EMBED_ASSETS
void vfs_init(void);
#endif /* EMBED_ASSETS */

#define INTERVAL 0.001

typedef struct vec2 {
	int x, y;
} Vec2;

static void tick(void);
static void test_event(unsigned long, void *);
static void test_collision(int, int, void *);
static void delete_on_collision(int, int, void *);

static int main_font;
static GameState *game_state;
static Audio *audio;
static int test_event_ctx;
static Dict *entity_dict;

int
main(void)
{
	Gc *gc;
	Image *img;
	GameState state;
	int x, y, player, npc;
	EntityInfo e;
	enum loglvl logging_level;

	logging_level = LOGLVL_TRACE; /* TODO arg parse */
	switch (logging_level) {
	case LOGLVL_WARNING:
		log_add_fd_sink(1, LOGMSK_WARNING);
		break;
	case LOGLVL_INFO:
		log_add_fd_sink(1, LOGMSK_WARNING | LOGMSK_INFO);
		break;
	case LOGLVL_DEBUG:
		log_add_fd_sink(1, LOGMSK_WARNING | LOGMSK_INFO | LOGMSK_DEBUG);
		break;
	default:
		log_add_fd_sink(1, LOGMSK_ALL ^ (LOGMSK_ERROR | LOGMSK_FATAL));
	};
	log_add_fd_sink(2, LOGMSK_ERROR | LOGMSK_FATAL);
	LOG_INFO("version: %s build: %s", VERSION, BUILD_INFO);

#ifdef EMBED_ASSETS
	vfs_init();
#endif /* EMBED_ASSETS */

	LOG_INFO("initialising graphical context");
	gc = gc_new();
	if (gc == NULL) {
		LOG_ERROR("failed at allocating graphical context");
		return 1;
	}
	gc_init(gc);
	audio_init();
	audio = audio_create();
	audio_load(audio, "blip", "assets/blip.snd");
	game_state = &state;
	state.gc = gc;
	state.audio = audio;
	state.prev = NULL;
	state.entity_manager = create_entity_manager();
	if (state.entity_manager == NULL) {
		LOG_ERROR("failed at allocating entity manager");
		return 1;
	}
	/* create a dictionary for notable entities */
	entity_dict = dict_create(DICT_DEFAULT_TABLE_SIZE);
	if (!entity_dict) {
		LOG_ERROR("failed to create a dictionary for entities");
		return 1;
	}

	memset(&e, 0, sizeof(EntityInfo));
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
	e.components = (COMPONENT_DIM | COMPONENT_POS | COMPONENT_VEL | COMPONENT_ACC | COMPONENT_ANIM | COMPONENT_ZPOS | COMPONENT_SPRITE | COMPONENT_INPUT);
	player = entity_spawn(state.entity_manager, e);
	dict_put(entity_dict, "player", &player);

	/* spawn npc */
	e.x = 60000;
	e.y = 20000;
	e.components = (COMPONENT_DIM | COMPONENT_POS | COMPONENT_ZPOS | COMPONENT_SPRITE);
	npc = entity_spawn(state.entity_manager, e);
	set_collsion(state.entity_manager, player, npc, test_collision, NULL);

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
	free(img->d);
	free(img);
	for (y = 0; y < 16; ++y) {
		for (x = 0; x < 20; ++x) {
			e.x = 6400 * x;
			e.y = 6400 * y;
			entity_spawn(state.entity_manager, e);
		}
	}

	gc_bind_input(gc);

	schedule(8000, &test_event, &test_event_ctx);

	//gc_set_resolution(gc, 1280, 960);
	//gc_set_resolution(gc, 960, 720);
	while (gc_alive(gc)) {
		while (gc_check_timer(INTERVAL))
			tick();
		gc_clear(gc);
		process_rendering(&state);
		gc_print(gc, main_font, 32, 400, 1, "> Hello world!\n\"The Legend of Tux\"\nZelda-like game test", 0);
		gc_commit(gc);
		audio_flush();
	}

	audio_exit();
	return 0;
}

static void
tick()
{
	static unsigned long ntick = 0;
	process_tick(game_state);
	collision_poll(ntick);
	schedule_poll(++ntick);
}

static void
test_event(unsigned long now, void *ctx)
{
	EntityInfo e;
	int *player, item;

	e.sprite = *(int *)ctx;
	e.z = 5;
	e.components = (COMPONENT_DIM | COMPONENT_POS | COMPONENT_ZPOS | COMPONENT_SPRITE);
	e.x = 70000;
	e.y = 20000;
	e.w = e.h = 3800;
	item = entity_spawn(game_state->entity_manager, e);
	LOG_INFO("spawned at %zu tick (%d x %d)", now, e.x, e.y);
	player = dict_lookup(entity_dict, "player");
	if (!player) {
		LOG_WARNING("player entity not found");
		return;
	}
	set_collsion(game_state->entity_manager, *player, item, delete_on_collision, NULL);
}

static void
test_collision(int first, int second, void *ctx)
{
	LOG_INFO("entity #%d collided with #%d", first, second);
	LOG_INFO("spawned txt %d", entity_spawn_text(game_state->entity_manager, main_font, 40000, 85000, "It's dangerous to go alone.\nTake this!", 38));
	audio_play(audio, "blip", 1.f);
}

static void
delete_on_collision(int first, int second, void *ctx)
{
	LOG_INFO("removing entity #%d", second);
	entity_delete(game_state->entity_manager, second);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
	audio_play(audio, "blip", 1.f);
}
