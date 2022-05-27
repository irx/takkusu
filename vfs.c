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
 * Virtual file system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "dict.h"

#include "assets_data.gen.h"

#define MAX_FDS 10

static struct vfile {
	size_t siz;
	void *p;
} files[NFILES];

static struct {
	struct vfile *p;
	size_t seek;
	int occup;
} file_descriptors[MAX_FDS];

static Dict *file_map;

static void add_asset(const char *, void *, size_t);

static void
add_asset(const char *filename, void *data, size_t siz)
{
	static size_t n = 0;

	files[n].p = data;
	files[n].siz = siz;
	dict_put(file_map, filename, &files[n++]);
}

void
vfs_init(void)
{
	file_map = dict_create(2048);
	ADD_ASSETS();
}

int
aopen(const char *filename, int flags)
{
	int fd;
	void *p;
	struct vfile *file;

	p = dict_lookup(file_map, filename);
	if (!p) {
		LOG_ERROR("file `%s' not found", filename);
		return -1;
	}
	file = (struct vfile *)p;
	for (fd = 0; fd < MAX_FDS; ++fd)
		if (!file_descriptors[fd].occup) {
			file_descriptors[fd].occup = 1;
			file_descriptors[fd].p = file;
			file_descriptors[fd].seek = 0;
			return fd;
		}

	LOG_ERROR("reached limit of virtual file descriptors");
	return -1;
}

ssize_t
aread(int fd, void *buf, size_t count)
{
	ssize_t n;
	struct vfile *file;
	unsigned char *src;

	if (!file_descriptors[fd].occup) {
		LOG_ERROR("invalid file descriptor %d", fd);
		return 0;
	}
	file = file_descriptors[fd].p;
	src = file->p;
	n = file->siz - file_descriptors[fd].seek;
	n = (n < count) ? n : count;
	memcpy(buf, &src[file_descriptors[fd].seek], n);
	file_descriptors[fd].seek += n;

	return n;
}
