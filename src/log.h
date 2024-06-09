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

#ifndef ENABLE_LOG_TRACE
# define LOG_TRACE(...)
#else
# define LOG_TRACE(...) log_print(LOGLVL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#endif /* ENABLE_LOG_TRACE */
#define LOG_DEBUG(...) log_print(LOGLVL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) log_print(LOGLVL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) log_print(LOGLVL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_print(LOGLVL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) {log_print(LOGLVL_FATAL, __FILE__, __LINE__, __VA_ARGS__); exit(1);}

#define LOG_PERROR(msg) log_perror(__FILE__, __LINE__, msg)

enum loglvl {
	LOGLVL_TRACE = 0,
	LOGLVL_DEBUG,
	LOGLVL_INFO,
	LOGLVL_WARNING,
	LOGLVL_ERROR,
	LOGLVL_FATAL,
	NLOGLVLS
};

enum logmsk {
	LOGMSK_TRACE   = 1 << LOGLVL_TRACE,
	LOGMSK_DEBUG   = 1 << LOGLVL_DEBUG,
	LOGMSK_INFO    = 1 << LOGLVL_INFO,
	LOGMSK_WARNING = 1 << LOGLVL_WARNING,
	LOGMSK_ERROR   = 1 << LOGLVL_ERROR,
	LOGMSK_FATAL   = 1 << LOGLVL_FATAL,
	LOGMSK_ALL     = 0xff
};

int log_add_fd_sink(int, enum logmsk);
void log_print(enum loglvl, const char *, int, const char *, ...);
void log_perror(const char *, int, const char *);
void log_update_progress(unsigned int);
void log_set_progressbar(unsigned int, const char *);
