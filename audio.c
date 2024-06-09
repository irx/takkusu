/**
 * Copyright (c) 2024 Max Mruszczak <u at one u x dot o r g>
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
 * Audio system
 */

#include <stdint.h>
#include <stdlib.h>

//#include <portaudio.h>

#include "log.h"
#include "audio.h"
#include "dict.h"

#define MIXER_BUFSIZ 65536

typedef struct samples Samples;
struct samples {
	Samples *next;
	int16_t *data;
	size_t size;
};

struct audio {
	Dict *map;
	Samples *shead, *stail;
};

static int16_t mixer_buf[MIXER_BUFSIZ];
static size_t mixer_buf_index;

Audio *
audio_create(void)
{
	Audio *a;

	a = malloc(sizeof(Audio));
	if (!a)
		return NULL;
	a->shead = a->stail = NULL;
	a->map = dict_create(DICT_DEFAULT_TABLE_SIZE);
	if (!a->map) {
		free(a);
		return NULL;
	}

	return a;
}

void
audio_destroy(Audio *audio)
{
	Samples *s;

	/* deallocate all audio tracks sample buffers */
	for (s = audio->shead; s != NULL; s = audio->shead) {
		audio->shead = s->next;
		free(s);
	}
	dict_destroy(audio->map);
	free(audio);
}

/**
 * Load samples from file
 */
void
audio_load(Audio *audio, const char *name, const char *path)
{
	Samples *s;

	s = malloc(sizeof(Samples));
	if (!s)
		LOG_FATAL("failed allocating mem for audio samples");
	s->next = NULL;
	s->size = 0;
	//s->size = snd_load(&(s->data), path);
	dict_put(audio->map, name, s);
	if (!audio->shead) {
		audio->shead = audio->stail = s;
		return;
	}
	audio->stail->next = s;
	audio->stail = s;
}

/**
 * Add a pre-loaded audio track to the global audio mixer buffer
 */
void
audio_play(Audio *audio, const char *name, float volume)
{
	Samples *s;
	size_t i;

	s = (Samples *)dict_lookup(audio->map, name);
	if (!s) {
		LOG_WARNING("audio track %s missing", name);
		return;
	}
	if (s->size == 0) {
		LOG_WARNING("audio track %s has 0 length", name);
		return;
	}
	/* add samples to the mixer buffer */
	for (i = mixer_buf_index; i < s->size; ++i)
		mixer_buf[i % MIXER_BUFSIZ] += (int16_t)(s->data[i] * volume);
}

int
audio_init(void)
{
	/* TODO */
	return 0;
}

/*
static int
portaudio_cb(
	const void *inbuf, void *outbuf,
	unsigned long frames_per_buffer,
	const PaStreamCallbackTimeInfo *time_info,
	PaStreamCallbackFlags flags,
	void *ctx)
{
	float *out;
	unsigned long i;

	(void)ctx;
	(void)inbuf;
	(void)time_info;
	(void)flags;

	out = (float *)outbuf;
	for (i = 0; i < frames_per_buffer; ++i) {
		*out++ = (float)mixer_buf[mixer_buf_index];
		mixer_buf[mixer_buf_index] = 0;
		mixer_buf_index = (mixer_buf_index + 1) % MIXER_BUFSIZ;
	}

	return 0;
}
*/
