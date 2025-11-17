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
 * Generalised input/output streams
 */

#include "u.h"
#include "io.h"
#include "log.h"


Stream *
io_open(const char *name, int flags)
{
	return nil;
}

ssize
io_read(Stream *s, void *dst, usize len)
{
	if (!s)
		LOG_FATAL("stream is nil");
	if (!s->vtable)
		LOG_FATAL("invalid stream");
	if (!s->vtable->read)
		LOG_FATAL("stream reader unimplemented");
	return s->vtable->read(s, dst, len);
}

ssize
io_write(Stream *s, const void *src, usize len)
{
	if (!s)
		LOG_FATAL("stream is nil");
	if (!s->vtable)
		LOG_FATAL("invalid stream");
	if (!s->vtable->write)
		LOG_FATAL("stream writer unimplemented");
	return s->vtable->write(s, src, len);
}

int
io_close(Stream *s)
{
	if (!s)
		LOG_FATAL("stream is nil");
	if (!s->vtable)
		LOG_FATAL("invalid stream");
	if (!s->vtable->close)
		LOG_FATAL("stream closer unimplemented");
	return s->vtable->close(s);
}

ssize
io_seek(Stream *s, ssize n, int type)
{
	if (!s)
		LOG_FATAL("stream is nil");
	if (!s->vtable)
		LOG_FATAL("invalid stream");
	if (!s->vtable->close)
		LOG_FATAL("stream seeker unimplemented");
	return s->vtable->seek(s, n, type);
}
