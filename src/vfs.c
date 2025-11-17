/**
 * Copyright (c) 2022-2025 Max Mruszczak <u at one u x dot o r g>
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

#include "u.h"
#include "log.h"
#include "io.h"
#include "dict.h"


#ifndef EMBED_ASSETS
# define NFILES 1 /* ISO C forbids zero-size array */
# define ADD_ASSETS() LOG_DEBUG("using local file system for asset loading");
#else
# include "assets_data.gen.h"
#endif /* EMBED_ASSETS */

#define MAX_FDS 10


static ssize vfs_read(Stream *, void *, usize);
static int vfs_close(Stream *);
/* TODO
static ssize vfs_write(Stream *, const void *, usize);
static ssize vfs_seek(Stream *, ssize, int);
*/

static struct vfile {
	size_t siz;
	void *p;
} files[NFILES];

static struct vfs_fd {
	Stream stream;
	struct vfile *p;
	size_t pos;
} file_descriptors[MAX_FDS];

static Dict *file_map;

static struct stream_vtable vfs_vtable = {
	.read = vfs_read,
	.write = nil,
	.close = vfs_close,
	.seek = nil
};

void
vfs_add_file(const char *filename, void *data, size_t siz)
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

Stream *
vfs_open(const char *filename, int flags)
{
	int fd;
	void *p;
	struct vfs_fd *vfd;
	struct vfile *file;

	p = dict_lookup(file_map, filename);
	if (!p) {
		LOG_ERROR("file `%s' not found", filename);
		return nil;
	}
	file = (struct vfile *)p;
	for (fd = 0; fd < MAX_FDS; ++fd)
		if (!file_descriptors[fd].stream.vtable) {
			vfd = &file_descriptors[fd];
			vfd->stream.vtable = &vfs_vtable;
			vfd->p = file;
			vfd->pos = 0;
			return (Stream *)vfd;
		}

	LOG_ERROR("reached limit of virtual file descriptors");
	return nil;
}

static ssize
vfs_read(Stream *s, void *buf, usize len)
{
	ssize_t n;
	struct vfs_fd *vfd;
	struct vfile *file;
	unsigned char *src;

	vfd = (struct vfs_fd *)s;
	if (vfd->stream.vtable != &vfs_vtable) {
		LOG_ERROR("passed invalid vfs stream");
		return 0;
	}
	file = vfd->p;
	src = file->p;
	n = file->siz - vfd->pos;
	n = (n < len) ? n : len;
	memcpy(buf, &src[vfd->pos], n);
	vfd->pos += n;

	return n;
}

static int
vfs_close(Stream *s)
{
	struct vfs_fd *vfd, *lower, *upper;

	vfd = (struct vfs_fd *)s;
	lower = &file_descriptors[0];
	upper = &file_descriptors[MAX_FDS];
	if (vfd < lower || vfd > upper)
		LOG_FATAL("attempted to close a non-vfs stream with vfs_close");
	memset(vfd, 0, sizeof(struct vfs_fd));

	return 0;
}
