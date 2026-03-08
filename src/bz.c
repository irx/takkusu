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
 * bzip2 decompression stream
 */


#include <bzlib.h>

#include "u.h"
#include "log.h"
#include "io.h"
#include "bz.h"


#define MAX_STREAMS 32
#define BZ_BUFSIZ BUFSIZ


static ssize bz_read(Stream *, void *, usize);
static int bz_close(Stream *);

static struct bz_stream {
	Stream stream, *ins;
	bz_stream bzs;
	char buf[BZ_BUFSIZ];
} bz_streams[MAX_STREAMS];

static struct stream_vtable bz_vtable = {
	.read = bz_read,
	.write = nil,
	.close = bz_close,
	.seek = nil
};


Stream *
bz_open(Stream *in_stream, int flags)
{
	int i;

	if (flags != IO_RDONLY)
		LOG_FATAL("only read bz streams are implemented");
	if (!in_stream)
		LOG_FATAL("refusing to decompress a nil stream");
	for (i = 0; i < MAX_STREAMS; ++i)
		if (!bz_streams[i].stream.vtable)
			break;
	if (i >= MAX_STREAMS) {
		LOG_WARNING("reached open bz streams limit");
		return nil;
	}
	bz_streams[i].bzs.bzalloc = nil;
	bz_streams[i].bzs.bzfree = nil;
	bz_streams[i].bzs.opaque = nil;
	if (BZ2_bzDecompressInit(&bz_streams[i].bzs, 0, 0) != BZ_OK) {
		LOG_ERROR("failed to init bz stream");
		return nil;
	}
	bz_streams[i].ins = in_stream;
	bz_streams[i].stream.vtable = &bz_vtable;

	return &bz_streams[i].stream;
}

static ssize
bz_read(Stream *s, void *dst, usize len)
{
	struct bz_stream *bs;
	ssize nread;
	usize rlimit = 10; /* limit decompress calls in case input data is missing */

	bs = (struct bz_stream *)s;
	bs->bzs.next_out = dst;
	bs->bzs.avail_out = len;
	while (rlimit) {
		if (bs->bzs.avail_in == 0) {
			nread = io_read(bs->ins, bs->buf, BZ_BUFSIZ);
			if (nread < 0)
				return nread;
			bs->bzs.avail_in = nread;
			bs->bzs.next_in = bs->buf;
		}
		switch (BZ2_bzDecompress(&bs->bzs)) {
		case BZ_PARAM_ERROR:
			LOG_WARNING("bz stream is nil or attempted to read 0 bytes");
			return -1;
		case BZ_DATA_ERROR:
			LOG_WARNING("bz data integrity error");
			return -1;
		case BZ_DATA_ERROR_MAGIC:
			LOG_ERROR("invalid bz magic bytes");
			return -1;
		case BZ_MEM_ERROR:
			LOG_FATAL("not enough memory for bz stream decompression");
		case BZ_OK:
			if (bs->bzs.avail_out > 0) {
				if (bs->bzs.avail_in == 0)
					--rlimit;
				continue;
			}
			/* fallthrough */
		case BZ_STREAM_END:
			return len - bs->bzs.avail_out;
		}
	}
	LOG_ERROR("couldn't reach end of stream; bz data likely incomplete");
	return -1;
}

static int
bz_close(Stream *s)
{
	struct bz_stream *bs;

	bs = (struct bz_stream *)s;
	if (bs->stream.vtable)
		return 1;
	BZ2_bzDecompressEnd(&bs->bzs); /* TODO: handle errors */
	io_close(bs->ins); /* TODO: make cascade closing optional? */
	bs->stream.vtable = nil;
	bs->ins = nil;

	return 0;
}
