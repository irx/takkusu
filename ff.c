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
# define aopen(...) open(__VA_ARGS__)
# define aread(...) read(__VA_ARGS__)
# define aclose(...) close(__VA_ARGS__)
#else
int aopen(const char *, int);
ssize_t aread(int, void *, size_t);
# define aclose(...)
#endif /* EMBED_ASSETS */

#define MUL_BOUND(x, y) (y > INT_MAX / x || y < INT_MIN / x)

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
