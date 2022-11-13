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

#ifdef _WIN32 /* definitions in `compat/w32io.c' */
int dprintf(int, const char *, ...);
#undef isatty
int isatty(int);
#endif /* _WIN32 */

#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"

#define MAX_SINKS 4
#define MAX_LEN 1024
#define PROGRESSBAR_LEN 20
#define PROGRESS_INFO_MAX_LEN 64

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

static size_t nsinks;

static struct {
	size_t current, target;
	char info[PROGRESS_INFO_MAX_LEN];
} progressbar;

static void
draw_progressbar(void)
{
	size_t barlen, i;

	if (!progressbar.target || progressbar.current > progressbar.target || !isatty(1))
		return;
	barlen = PROGRESSBAR_LEN * progressbar.current / progressbar.target;
	dprintf(1, "   \x1b[32m");
	for (i = 0; i < barlen; ++i)
		dprintf(1, "─");
	dprintf(1, "\x1b[30m");
	for (; i < PROGRESSBAR_LEN; ++i)
		dprintf(1, "─");
	dprintf(1, "\x1b[32m  %zu/%zu \x1b[0;1m%s\x1b[0m\r",
		progressbar.current,
		progressbar.target,
		progressbar.info);
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
log_print(enum loglvl lvl, const char *file, int line, const char *msg, ...)
{
	int i;
	struct tm tm;
	time_t now;
	va_list ap;
	char buf[MAX_LEN];

	va_start(ap, msg);
	vsnprintf(buf, MAX_LEN, msg, ap);
	va_end(ap);
	if (nsinks < 1) {
		dprintf(2, "no log sinks; dropped message: %s\n", buf);
		return;
	}
	now = time(NULL);
	gmtime_r(&now, &tm);
	for (i = 0; i < nsinks; ++i) {
		if (!((sinks.mask[i] >> lvl) & 1))
			continue;
		dprintf(sinks.fd[i], log_msg[lvl][sinks.fmt[i]],
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec,
			file, line, buf);
	}
	draw_progressbar();
}

void
log_perror(const char *file, int line, const char *msg)
{
	log_print(LOGLVL_ERROR, file, line, "%s: %s", msg, strerror(errno));
}

void
log_update_progress(unsigned int current)
{
	progressbar.current = current;
	draw_progressbar();
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
	draw_progressbar();
}
