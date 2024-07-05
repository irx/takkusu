/**
 * Copyright (c) 2021 Max Mruszczak <u at one u x dot o r g>
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
 * Read farbfeld image
 */

#include <unistd.h>
#ifndef _WIN32
# include <arpa/inet.h>
#else
# include <winsock.h>
#endif /* _WIN32 */
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "ff.h"

#ifndef EMBED_ASSETS
# ifndef _WIN32
#  define aopen(...) open(__VA_ARGS__)
# else
#  define aopen(path, flags) open(path, flags | O_BINARY)
# endif /* _WIN32 */
# define aread(...) read(__VA_ARGS__)
# define aclose(...) close(__VA_ARGS__)
#else
int aopen(const char *, int);
ssize_t aread(int, void *, size_t);
# define aclose(...)
#endif /* EMBED_ASSETS */

#define SAMPLE_RATE 44100 /* expected sample rate of audio file read by `snd_load' */
#define MUL_BOUND(x, y) (y > INT_MAX / x || y < INT_MIN / x)

typedef struct {
	size_t offset, size, rate, chan;
	enum {
		LINEAR16 = 3
	} encoding;
} SndHdr;

static int snd_read_header(int, SndHdr *);

Image *
ff_load(const char *path)
{
	int fd;
	uint8_t hdr[16];
	uint16_t *rowbuf;
	size_t rowsiz, i, j, offs, cur;
	Image *img;

	LOG_DEBUG("loading image asset: %s", path);
	if ((fd = aopen(path, O_RDONLY)) < 0) {
		LOG_PERROR("couldn't open file");
		return NULL;
	}
	if (aread(fd, hdr, 16) != 16) {
		LOG_PERROR("couldn't read header");
		goto err1;
	}
	if (memcmp(hdr, "farbfeld", 8)) {
		LOG_PERROR("not a farbfeld file");
		goto err1;
	}

	if (!(img = calloc(sizeof(Image), 1))) {
		LOG_PERROR("failed to allocate image struct");
		goto err1;
	}
	img->w = ntohl(((uint32_t *)hdr)[2]);
	img->h = ntohl(((uint32_t *)hdr)[3]);
	/*
	if (MUL_BOUND(img->w, img->h)) {
		LOG_PERROR("image size out of boundaries");
		goto err2;
	}
	*/
	img->siz = img->w * img->h * 4;
	if (!(img->d = calloc(sizeof(float), img->siz))) {
		LOG_PERROR("failed to allocate image struct");
		goto err2;
	}

	rowsiz = sizeof(uint16_t) * 4 * img->w;
	if (!(rowbuf = calloc(rowsiz, 1))) {
		LOG_PERROR("failed to allocate rowbuf struct");
		goto err3;
	}

	cur = 0;
	for (i = 0; i < img->h; ++i) {
		if (aread(fd, rowbuf, rowsiz) != rowsiz) {
			LOG_PERROR("failed to read image row");
			free(rowbuf);
			goto err3;
		}
		for (j = 0; j < img->w; ++j) {
			offs = j * 4;
			img->d[cur++] = (float)ntohs(rowbuf[offs]) / 65536.f;
			img->d[cur++] = (float)ntohs(rowbuf[offs+1]) / 65536.f;
			img->d[cur++] = (float)ntohs(rowbuf[offs+2]) / 65536.f;
			img->d[cur++] = (float)ntohs(rowbuf[offs+3]) / 65536.f;
		}
	}

	free(rowbuf);
	aclose(fd);
	return img;


err3:	free(img->d);
err2:	free(img);
err1:	aclose(fd);
	return NULL;
}

size_t
snd_load(int16_t **dst, const char *path)
{
	int fd, i;
	size_t len;
	ssize_t r;
	SndHdr hdr;

	fd = aopen(path, O_RDONLY);
	if (fd < 0) {
		LOG_ERROR("couldn't open %s", path);
		return 0;
	}
	if (snd_read_header(fd, &hdr) < 0) {
		LOG_ERROR("couldn't read header for %s", path);
		return 0;
	}
	if (hdr.encoding != LINEAR16) {
		LOG_ERROR("encoding of `%s' is not LINEAR16", path);
		return 0;
	}
	if (hdr.rate != SAMPLE_RATE) {
		LOG_ERROR("sampling rate of `%s' is %zu; only %zu is supported for now", path, hdr.rate, SAMPLE_RATE);
		return 0;
	}
	if (hdr.size < 0) {
		len = SAMPLE_RATE * 10;
		LOG_WARNING("audio file `%s' does not specify its size; defaulting to %zu", path, len);
	} else
		len = hdr.size;
	*dst = malloc(sizeof(int16_t)*len);
	if (!*dst) {
		LOG_ERROR("couldn't allocate audio samples memeory");
		return 0;
	}
	r = aread(fd, *dst, sizeof(int16_t)*len);
	if (r < 0) {
		LOG_PERROR("error loading snd samples");
		free(*dst);
		return 0;
	}
	if (r == 0) {
		LOG_WARNING("loaded 0 samples for `%s'", path);
		free(*dst);
		return 0;
	}
	if (r < sizeof(int16_t)*len) /* TODO trim overallocated space */
		len = r / sizeof(int16_t);
	LOG_DEBUG("read %zu samples from `%s'", len, path);
	for (i = 0; i < len; ++i)
		(*dst)[i] = ntohs((*dst)[i]);

	return len;
}

static int
snd_read_header(int fd, SndHdr *hdr)
{
	ssize_t r;
	uint32_t buf[5];
	uint8_t sign[4];

	r = aread(fd, sign, 4);
	if (r != 4) {
		return -1;
	}
	if (sign[0] != '.' || sign[1] != 's' || sign[2] != 'n' || sign[3] != 'd')
	{
		return -1;
	}
	r = aread(fd, buf, sizeof(buf));
	if (r != sizeof(buf)) {
		return -1;
	}
	hdr->offset = (size_t)ntohl(buf[0]);
	if (hdr->offset < 24) {
		LOG_ERROR("snd data offset is smaller than the header: %zu", hdr->offset);
		return -1;
	}
	hdr->size = (size_t)ntohl(buf[1]);
	hdr->encoding = ntohl(buf[2]);
	hdr->rate = (size_t)ntohl(buf[3]);
	hdr->chan = (size_t)ntohl(buf[4]);
	/* TODO seek based on offset */

	return 0;
}
