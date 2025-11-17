/**
 * Copyright (c) 2025 Max Mruszczak <u at one u x dot o r g>
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
 * Local filesystem i/o
 */

#include <unistd.h>
#ifndef _WIN32
# include <arpa/inet.h>
#else
# include <winsock.h>
#endif /* _WIN32 */
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u.h"
#include "log.h"
#include "io.h"
#include "fs.h"

#ifndef _WIN32
# define aopen(...) open(__VA_ARGS__)
#else
# define aopen(path, flags) open(path, flags | O_BINARY)
#endif /* _WIN32 */
#define aread(...) read(__VA_ARGS__)
#define aclose(...) close(__VA_ARGS__)

#define MAX_STREAMS 32


static ssize fs_read(Stream *, void *, usize);
static ssize fs_write(Stream *, const void *, usize);
static int fs_close(Stream *);
static ssize fs_seek(Stream *, ssize, int);

static struct fs_stream {
	Stream stream;
	int fd;
} fs_streams[MAX_STREAMS];

static struct stream_vtable fs_vtable = {
	.read = fs_read,
	.write = fs_write,
	.close = fs_close,
	.seek = fs_seek
};


Stream *
fs_open(const char *name, int flags)
{
	int i;

	if (flags != IO_RDONLY) /* allow reading only for now */
		LOG_FATAL("filesystem is read only");
	for (i = 0; i < MAX_STREAMS; ++i)
		if (!fs_streams[i].stream.vtable)
			break;
	if (i >= MAX_STREAMS) {
		LOG_WARNING("reached open fs streams limit");
		return nil;
	}
	fs_streams[i].fd = aopen(name, O_RDONLY);
	if (fs_streams[i].fd < 0) {
		LOG_PERROR("fs stream open error");
		return nil;
	}
	fs_streams[i].stream.vtable = &fs_vtable;

	return &fs_streams[i].stream;
}

static ssize
fs_read(Stream *s, void *dst, usize len)
{
	struct fs_stream *fs;

	fs = (struct fs_stream *)s;
	return aread(fs->fd, dst, len);
}

static ssize
fs_write(Stream *s, const void *src, usize len)
{
	//struct fs_stream *fs;

	//fs = (struct fs_stream *)s;
	//return awrite(fs->fd, src, len);
	return -1;
}

static int
fs_close(Stream *s)
{
	struct fs_stream *fs;

	fs = (struct fs_stream *)s;
	if (fs->stream.vtable)
		return 1;
	aclose(fs->fd);
	fs->stream.vtable = nil;

	return 0;
}

static ssize
fs_seek(Stream *s, ssize n, int type)
{
	int whence;
	struct fs_stream *fs;

	switch (type) {
	case IO_SET: whence = SEEK_SET; break;
	case IO_CUR: whence = SEEK_CUR; break;
	case IO_END: whence = SEEK_END; break;
	default: LOG_FATAL("invalid seek type: %d", type);
	}
	fs = (struct fs_stream *)s;
	if (fs->stream.vtable)
		return -1;
	return lseek(fs->fd, n, whence);
}
