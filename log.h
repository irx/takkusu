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

#define LOG_TRACE(...) log_print(TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_print(DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) log_print(INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) log_print(WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_print(ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_print(FATAL, __FILE__, __LINE__, __VA_ARGS__); exit(1)

#define LOG_PERROR(msg) log_perror(__FILE__, __LINE__, msg)

enum loglvl {
	TRACE = 0,
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	FATAL,
	NLOGLVLS
};

int log_add_fd_sink(int, enum loglvl);
void log_print(enum loglvl, const char *, int, const char *, ...);
void log_perror(const char *, int, const char *);
