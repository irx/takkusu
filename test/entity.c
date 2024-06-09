#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/log.h"
#include "../src/dict.h"
#include "../src/ff.h"
#include "../src/render.h"
#include "../src/entity.h"

int
main(void)
{
	GameState state;
	EntityInfo info;
	int entity;

	log_add_fd_sink(1, LOGMSK_ALL ^ (LOGMSK_ERROR | LOGMSK_FATAL));
	log_add_fd_sink(2, LOGMSK_ERROR | LOGMSK_FATAL);

	state.entity_manager = create_entity_manager();
	entity = entity_spawn(state.entity_manager, info);
	LOG_INFO("spawned entity #%d", entity);
	entity = entity_spawn(state.entity_manager, info);
	LOG_INFO("spawned entity #%d", entity);
	destroy_entity_manager(state.entity_manager);

	return 0;
}
