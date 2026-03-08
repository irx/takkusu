#include <stdio.h>
#include "../src/u.h"
#include "../src/io.h"
#include "../src/fs.h"
#include "../src/bz.h"

#define SIZ 32


int
main(void)
{
	ssize r;
	char buf[SIZ];
	Stream *in;

	in = fs_open("/tmp/testfile.bz2", IO_RDONLY);
	in = bz_open(in, IO_RDONLY);
	r = io_read(in, buf, SIZ - 1);
	io_close(in);
	if (r < 0 || r > SIZ) {
		printf("failed: %zd", r);
		return 1;
	}
	buf[r] = '\0';
	printf("read: %s\n", buf);

	return 0;
}
