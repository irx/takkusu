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
 * Render images
 */

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "log.h"
#include "ff.h"
#include "render.h"

#define SPRITE_LIMIT 512

/* SHADERS */
static const char *vert_shader_src =
	"#version 120\n"
	"attribute vec2 position;\n"
	"attribute vec2 texture;\n"
	"uniform mat4 tfm;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 offs;\n"
	"uniform float zpos;\n"
	"varying vec2 Texcoord;\n"
	"void main()\n"
	"{\n"
	"	Texcoord = texture*scale + offs*scale;\n"
	"	gl_Position = vec4(position, zpos/10 + position.y/100, 1.0) * tfm;\n"
	"}\n";

static const char *frag_shader_src =
	"#version 120\n"
	"varying vec2 Texcoord;\n"
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	vec4 colour = texture2D(tex, Texcoord);\n"
	"	gl_FragColor = colour;\n"
	"}\n";

struct gc {
	float *vert;
	GLuint vao, vbo, vertex_shader, fragment_shader, prog,
	       texture, position, tfm, tex_scale, tex_offs, tex_z;
	char *v_shd_src, *f_shd_src;
	GLuint sprites[SPRITE_LIMIT];
	size_t nsprites, spritew[SPRITE_LIMIT], spriteh[SPRITE_LIMIT];
	float sprite_scale[SPRITE_LIMIT][2];
	int w, h;
	GLFWwindow *window;
};

static void key_callback(GLFWwindow *, int, int, int, int);

static Input global_input;

static float vert[] = {
	 1.0f,  1.0f,  1.0f,  0.0f,
	 1.0f, -1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  0.0f,  1.0f,
	-1.0f,  1.0f,  0.0f,  0.0f
};

Gc *
gc_new(void)
{
	return malloc(sizeof(Gc));
}

int
gc_init(Gc *gc)
{
	char err[512];

	gc->nsprites = 0;
	gc->w = 640;
	gc->h = 480;

	gc->vert = malloc(sizeof(vert));
	if (gc->vert == NULL)
		return -1;
	memcpy(gc->vert, vert, sizeof(vert));
	gc->v_shd_src = strdup(vert_shader_src);
	gc->f_shd_src = strdup(frag_shader_src);

	if (!glfwInit())
		return -1;

	LOG_INFO("opening %dx%d window", gc->w, gc->h);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	gc->window = glfwCreateWindow(gc->w, gc->h, "Hello World", NULL, NULL);

	if (!gc->window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(gc->window);
	glfwSwapInterval(1);
	glViewport(0, 0, gc->w, gc->h);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		LOG_ERROR("GL wrangler failed!");
		return -1;
	}

	glGenVertexArrays(1, &gc->vao);
	glBindVertexArray(gc->vao);
	glGenBuffers(1, &gc->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gc->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vert), gc->vert, GL_STATIC_DRAW);

	/* transparency */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.f, 1.f, 1.f, 1.f);
	//glEnable(GL_DEPTH_TEST);

	gc->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(gc->vertex_shader, 1, &vert_shader_src, NULL);
	glCompileShader(gc->vertex_shader);
	glGetShaderInfoLog(gc->vertex_shader, 512, NULL, err);
	LOG_ERROR("%s", err);

	gc->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(gc->fragment_shader, 1, &frag_shader_src, NULL);
	glCompileShader(gc->fragment_shader);
	glGetShaderInfoLog(gc->fragment_shader, 512, NULL, err);
	LOG_ERROR("%s", err);

	gc->prog = glCreateProgram();
	glAttachShader(gc->prog, gc->vertex_shader);
	glAttachShader(gc->prog, gc->fragment_shader);
	glLinkProgram(gc->prog);
	glUseProgram(gc->prog);

	gc->position = glGetAttribLocation(gc->prog, "position");
	gc->texture = glGetAttribLocation(gc->prog, "texture");
	glVertexAttribPointer(gc->position, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
	glVertexAttribPointer(gc->texture, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void *)(2*sizeof(float)));
	glEnableVertexAttribArray(gc->position);
	glEnableVertexAttribArray(gc->texture);

	gc->tfm = glGetUniformLocation(gc->prog, "tfm");
	gc->tex_scale = glGetUniformLocation(gc->prog, "scale");
	gc->tex_offs = glGetUniformLocation(gc->prog, "offs");
	gc->tex_z = glGetUniformLocation(gc->prog, "zpos");

	return 0;
}

int
gc_create_sprite(Gc *gc, const Image *img, unsigned int w, unsigned int h)
{
	GLuint tex;

	if (gc->nsprites > SPRITE_LIMIT)
		return -1;

	LOG_DEBUG("loading a sprite of size %dx%d", img->w, img->h);
	glUseProgram(gc->prog);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 0, GL_RGBA, GL_FLOAT, img->d);

	gc->sprites[gc->nsprites] = tex;
	gc->spritew[gc->nsprites] = w;
	gc->spriteh[gc->nsprites] = h;
	gc->sprite_scale[gc->nsprites][0] = (float)w/(float)img->w;
	gc->sprite_scale[gc->nsprites][1] = (float)h/(float)img->h;
	return gc->nsprites++;
}

void
gc_draw(Gc *gc, int sprite, int x, int y, int z, int ox, int oy)
{
	float tfm[] = {
		 1.0f,  0.0f,  0.0f,  0.0f,
		 0.0f,  1.0f,  0.0f,  0.0f,
		 0.0f,  0.0f,  1.0f,  0.0f,
		 0.0f,  0.0f,  0.0f,  1.0f
	};
	/* translate */
	tfm[3] = -1.f + (double)x/(double)gc->w;
	tfm[7] = 1.f - (double)y/(double)gc->h;
	/* scale */
	tfm[0] = (double)gc->spritew[sprite]/(double)gc->w;
	tfm[5] = (double)gc->spriteh[sprite]/(double)gc->h;
	glUseProgram(gc->prog);
	glBindTexture(GL_TEXTURE_2D, gc->sprites[sprite]);
	glUniformMatrix4fv(gc->tfm, 1, GL_FALSE, tfm);
	glUniform2fv(gc->tex_scale, 1, gc->sprite_scale[sprite]);
	glUniform2f(gc->tex_offs, (float)ox, (float)oy);
	glUniform1f(gc->tex_z, (float)z);
	glDrawArrays(GL_QUADS, 0, 4);
}

void
gc_print(Gc *gc, int sprite, int x, int y, int z, const char *s, size_t len)
{
	size_t i;
	int ox, oy, c;
	float tfm[] = {
		 1.0f,  0.0f,  0.0f,  0.0f,
		 0.0f,  1.0f,  0.0f,  0.0f,
		 0.0f,  0.0f,  1.0f,  0.0f,
		 0.0f,  0.0f,  0.0f,  1.0f
	};

	if (!len)
		len = strlen(s);
	/* translate */
	tfm[3] = -1.f + (float)x/(float)gc->w;
	tfm[7] = 1.f - (float)y/(float)gc->h;
	/* scale */
	tfm[0] = (float)gc->spritew[sprite]/(float)gc->w;
	tfm[5] = (float)gc->spriteh[sprite]/(float)gc->h;
	glUseProgram(gc->prog);
	glBindTexture(GL_TEXTURE_2D, gc->sprites[sprite]);
	glUniform2fv(gc->tex_scale, 1, gc->sprite_scale[sprite]);
	for (i = 0; i < len; ++i) {
		if (s[i] == '\n') {
			tfm[3] = -1.f + (float)x/(float)gc->w;
			tfm[7] -= 2.f * (float)gc->spriteh[sprite] / (float)gc->h;
			continue;
		}
		c = s[i] - 32;
		oy = c / 10;
		ox = c % 10;
		glUniform2f(gc->tex_offs, (float)ox, (float)oy);
		glUniformMatrix4fv(gc->tfm, 1, GL_FALSE, tfm);
		glDrawArrays(GL_QUADS, 0, 4);
		/* move one width to the right */
		//x += gc->spritew[sprite] * 2;
		//tfm[3] = -1.f + (float)x/(float)gc->w;
		tfm[3] += 2.f * (float)gc->spritew[sprite] / (float)gc->w;
	}
}

void
gc_clear(Gc *gc)
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void
gc_commit(Gc *gc)
{
	glfwSwapBuffers(gc->window);
	glfwPollEvents();
}

GLFWwindow *
gc_get_window(const Gc *gc)
{
	return gc->window;
}

int
gc_alive(const Gc *gc)
{
	return !glfwWindowShouldClose(gc->window);
}

void
gc_select(const Gc *gc)
{
	glfwMakeContextCurrent(gc->window);
}

void
gc_set_resolution(const Gc *gc, unsigned int width, unsigned int height)
{
	glfwSetWindowSize(gc->window, width, height);
	glViewport(0, 0, width, height);
}

int
gc_check_timer(double delta)
{
	double now;

	now = glfwGetTime();
	if (now < delta)
		return 0;
	glfwSetTime(now - delta);
	return 1;
}

void
gc_bind_input(const Gc *gc)
{
	glfwSetKeyCallback(gc_get_window(gc), key_callback);
}

Input
gc_poll_input(void)
{
	glfwPollEvents();
	return global_input;
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		global_input.dx = 1.f;
	if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		global_input.dx = 0.f;
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		global_input.dx = -1.f;
	if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		global_input.dx = 0.f;
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		global_input.dy = -1.f;
	if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		global_input.dy = 0.f;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		global_input.dy = 1.f;
	if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		global_input.dy = 0.f;
}
