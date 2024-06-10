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

enum component {
	COMPONENT_DIM     = 1 << 0,
	COMPONENT_POS     = 1 << 1,
	COMPONENT_VEL     = 1 << 2,
	COMPONENT_ACC     = 1 << 3,
	COMPONENT_ZPOS    = 1 << 4,
	COMPONENT_SPRITE  = 1 << 5,
	COMPONENT_ANIM    = 1 << 6,
	COMPONENT_TEXT    = 1 << 7,
	COMPONENT_INPUT   = 1 << 8
};

typedef struct entity_manager EntityManager;
typedef struct game_state GameState;
struct game_state {
	GameState *prev;
	Gc *gc;
	EntityManager *entity_manager;
};

typedef struct {
	enum component components;
	int x, y, z, w, h, sprite;
	const char *txt;
} EntityInfo;

EntityManager * create_entity_manager(void);
void destroy_entity_manager(EntityManager *);
int entity_spawn(EntityManager *, EntityInfo);
int entity_spawn_text(EntityManager *, int, int, int, const char *, int);
int entity_get_info(EntityManager *, int, EntityInfo *);
void entity_delete(EntityManager *, int);
void process_tick(GameState *);

int entity_detect_collision(EntityManager *, int, int, void (*fn)(int, int, void *), void *);
