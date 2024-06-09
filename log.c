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
 * Simple logging util
 */

#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32 /* definitions in `compat/w32io.c' */
int dprintf(int, const char *, ...);
ssize_t __write_w32(int fd, const char *buf, size_t len);
# undef write
# define write __write_w32
# undef isatty
int isatty(int);
#endif /* _WIN32 */

#include "log.h"

#define MAX_SINKS 4
#define MAX_LINE_LEN 1024
#define LOG_BUFSIZ 4096
#define PROGRESSBAR_LEN 20
#define PROGRESS_INFO_MAX_LEN 64
#define PROGRESSBAR_SEGMENT "─"

#define LOG_MSG_TEMPL(lvl) \
	"%.4d-%.2d-%.2d %.2d:%.2d:%.2d [ " lvl " ] %s:%d %s\n"
#define LOG_MSG_TEMPL_COL(col, lvl) \
	"\33[1;34m%.4d-%.2d-%.2d %.2d:%.2d:%.2d \33[0;" col ";7m " lvl \
	" \33[0;1;36m %s:%d\33[0m %s\n"

static const char *log_msg[][2] = {
	{
		LOG_MSG_TEMPL("TRACE  "),
		LOG_MSG_TEMPL_COL("34", "TRACE  ")
	},
	{
		LOG_MSG_TEMPL("DEBUG  "),
		LOG_MSG_TEMPL_COL("32", "DEBUG  ")
	},
	{
		LOG_MSG_TEMPL("INFO   "),
		LOG_MSG_TEMPL_COL("0", "INFO   ")
	},
	{
		LOG_MSG_TEMPL("WARNING"),
		LOG_MSG_TEMPL_COL("33", "WARNING")
	},
	{
		LOG_MSG_TEMPL("ERROR  "),
		LOG_MSG_TEMPL_COL("31", "ERROR  ")
	},
	{
		LOG_MSG_TEMPL("FATAL  "),
		LOG_MSG_TEMPL_COL("35", "FATAL  ")
	}
};

static struct {
	int fd[MAX_SINKS], fmt[MAX_SINKS];
	enum logmsk mask[MAX_SINKS];
} sinks;

static struct {
	char d[2][LOG_BUFSIZ];
	ssize_t i[2];
} bufs;

static size_t nsinks;

static struct {
	size_t current, target;
	char info[PROGRESS_INFO_MAX_LEN];
} progressbar;

static size_t draw_progressbar(char *dst, size_t len);
static ssize_t log_write(int);


static size_t
draw_progressbar(char *dst, size_t len)
{
	size_t segmentlen, barlen, i, z;
	int r;

	if (!progressbar.target || progressbar.current > progressbar.target)
		return 0;
	segmentlen = sizeof(PROGRESSBAR_SEGMENT) - 1;
	if (len < PROGRESSBAR_LEN * segmentlen + PROGRESS_INFO_MAX_LEN + 64/* vt100 escape seq padding */)
		return 0;
	barlen = PROGRESSBAR_LEN * progressbar.current / progressbar.target;
	memcpy(dst, "   \x1b[32m", 8);
	z = 8;
	for (i = 0; i < barlen; ++i) {
		memcpy(&dst[z], PROGRESSBAR_SEGMENT, segmentlen);
		z += segmentlen;
	}
	memcpy(&dst[z], "\x1b[30m", 5);
	z += 5;
	for (; i < PROGRESSBAR_LEN; ++i) {
		memcpy(&dst[z], PROGRESSBAR_SEGMENT, segmentlen);
		z += segmentlen;
	}
	r = snprintf(&dst[z], len - z,
		"\x1b[32m  %zu/%zu \x1b[0;1m%s\x1b[0m\r",
		progressbar.current,
		progressbar.target,
		progressbar.info);
	if (r < 0)
		return 0;
	z += r;
	return z;
}

int
log_add_fd_sink(int fd, enum logmsk mask)
{
	if (nsinks + 1 >= MAX_SINKS) {
		perror("reached log sinks limit");
		return 0;
	}
	sinks.fmt[nsinks] = isatty(fd);
	sinks.mask[nsinks] = mask;
	sinks.fd[nsinks++] = fd;
	return 1;
}


void
log_print(enum loglvl lvl, const char *file, int line, const char *fmt, ...)
{
	int i;
	struct tm tm;
	time_t now;
	va_list ap;
	char msg_buf[MAX_LINE_LEN];

	va_start(ap, fmt);
	vsnprintf(msg_buf, MAX_LINE_LEN, fmt, ap);
	va_end(ap);
	if (nsinks < 1) {
		dprintf(2, "no log sinks; dropped message: %s\n", msg_buf);
		return;
	}
	now = time(NULL);
	gmtime_r(&now, &tm);
	/* for isatty == 0 */
	bufs.i[0] = snprintf(&bufs.d[0][0], LOG_BUFSIZ, log_msg[lvl][0],
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		file, line, msg_buf);
	/* for isatty == 1 (VT100 formatting) */
	bufs.i[1] = snprintf(&bufs.d[1][0], LOG_BUFSIZ, log_msg[lvl][1],
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		file, line, msg_buf);
	if (bufs.i[0] < 0 || bufs.i[1] < 0) {
		dprintf(2, "log buffering failed; dropped message: %s\n", msg_buf);
		return;
	}
	bufs.i[1] += draw_progressbar(&bufs.d[1][bufs.i[1]], LOG_BUFSIZ - bufs.i[1]);
	for (i = 0; i < nsinks; ++i) {
		if (!((sinks.mask[i] >> lvl) & 1))
			continue;
		log_write(i);
	}
}

void
log_perror(const char *file, int line, const char *msg)
{
	log_print(LOGLVL_ERROR, file, line, "%s: %s", msg, strerror(errno));
}

void
log_update_progress(unsigned int current)
{
	int i;

	progressbar.current = current;
	bufs.i[1] = draw_progressbar(&bufs.d[1][0], LOG_BUFSIZ);
	for (i = 0; i < MAX_SINKS; ++i)
		if (sinks.fmt[i]) {
			log_write(i);
			return;
		}
}

void
log_set_progressbar(unsigned int target, const char *info)
{
	size_t i;

	i = 0;
	if (info) {
		for (; i < PROGRESS_INFO_MAX_LEN - 1 && info[i] != '\0'; ++i)
			progressbar.info[i] = info[i];
	}
	for (; i < PROGRESS_INFO_MAX_LEN; ++i)
		progressbar.info[i] = '\0';
	progressbar.target = target;
	log_update_progress(0);
}

static ssize_t
log_write(int idx)
{
	int fd, fmt;
	ssize_t w, r;

	fd = sinks.fd[idx];
	fmt = sinks.fmt[idx];
	w = r = 0;
	while ((r = write(fd, &bufs.d[fmt][w], bufs.i[fmt] - w)) > 0)
		w += r;
	if (r < 0)
		return r;
	return w;
}
