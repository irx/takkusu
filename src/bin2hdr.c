/**
 * Copyright (c) 2022 Max Mruszczak <u at one u x dot o r g>
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
 * Convert binary asset to a C header
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS 10

static char * tr_name(const char *);

int
main(int argc, char *argv[])
{
	int fd;
	unsigned char buf;
	char *name;
	size_t i = 0;

	if (argc > 1) {
		name = tr_name(argv[1]);
		fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			perror("failed to open file");
			exit(1);
		}
		dprintf(1, "static const unsigned char %s[] = ", name);
	} else
		fd = 0;
	if (!read(fd, &buf, 1)) {
		/* empty file? */
		dprintf(1, "{}\n");
		return 0;
	}
	dprintf(1, "{\n\t0x%x", buf);
	while (read(fd, &buf, 1)) {
		if (++i % COLS)
			dprintf(1, ", 0x%.2x", (unsigned int)buf);
		else
			dprintf(1, ",\n\t0x%.2x", (unsigned int)buf);
	}
	dprintf(1, "\n};\n");

	if (argc > 1) {
		close(fd);
		free(name);
	}
	return 0;
}

static char *
tr_name(const char *file)
{
	char *name, *c;

	name = strdup(file);
	if (!name) {
		perror("name str alloc fail");
		exit(1);
	}
	for (c = name; *c != '\0'; ++c)
		switch (*c) {
		case '-':
		case ' ':
		case '.':
		case '/':
			*c = '_';
			break;
		}

	return name;
}
