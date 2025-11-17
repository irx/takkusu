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


enum io_flag {
	IO_RDONLY = 0x01,
	IO_WRONLY = 0x02,
	IO_RDWR = 0x03
};

enum io_seek_type {
	IO_SET = 0,
	IO_CUR,
	IO_END
};

typedef struct stream Stream;

struct stream_vtable {
	ssize (*read)(Stream *, void *, usize);
	ssize (*write)(Stream *, const void *, usize);
	int (*close)(Stream *);
	ssize (*seek)(Stream *, ssize, int);
};

struct stream {
	struct stream_vtable *vtable;
};

Stream * io_open(const char *, int);
ssize io_read(Stream *, void *, usize);
ssize io_write(Stream *, const void *, usize);
int io_close(Stream *);
ssize io_seek(Stream *, ssize, int);
