#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../dict.h"

static char *strs[] = {
	"random thing",
	"another one",
	"whatever"
};

int
main(void)
{
	Dict *d;
	int i;
	char tmp[512];

	d = dict_create(1024);

	assert(dict_size(d) == 0);
	dict_put(d, "thing", strs[1]);
	dict_put(d, "hello", strs[2]);
	assert(dict_size(d) == 2);
	assert(strcmp(dict_lookup(d, "thing"), strs[1]) == 0);
	assert(strcmp(dict_lookup(d, "hello"), strs[2]) == 0);

	for (i = 0; i < 10000; ++i) {
		snprintf(tmp, 512, "string no %d", i);
		dict_put(d, tmp, strs[0]);
		dict_put(d, "test", strs[0]);
		dict_put(d, "test", NULL);
		dict_prune(d);
	}
	assert(dict_size(d) == 10002);

	printf("im looking up: %s\n", (char *)dict_lookup(d, "thing"));
	printf("size of dict is: %lu\n", dict_size(d));
	dict_put(d, "thing", NULL);
	dict_put(d, "hello", NULL);
	printf("after deleting entries: %lu\n", dict_size(d));
	dict_prune(d);
	dict_prune(d);
	assert(dict_size(d) == 10000);
	dict_destroy(d);

	return 0;
}
