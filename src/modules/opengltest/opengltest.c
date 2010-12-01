/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glib.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "modules/opengl/opengl.h"
#include "modules/opengl/material.h"
#include "modules/opengl/shader.h"
#include "modules/opengl/mesh.h"
#include "modules/module_util/module_util.h"
#include "modules/event/event.h"

#include "api.h"

MODULE_NAME("opengltest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The opengltest module creates a simple OpenGL window sample");
MODULE_VERSION(0, 3, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 3, 0), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2));

static OpenGLWindow *window = NULL;
static OpenGLMesh *mesh = NULL;
static GLuint program = 0;

static void listener_mouseDown(void *subject, const char *event, void *data, va_list args);
static void listener_mouseUp(void *subject, const char *event, void *data, va_list args);
static void listener_keyDown(void *subject, const char *event, void *data, va_list args);
static void listener_keyUp(void *subject, const char *event, void *data, va_list args);
static void listener_display(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	bool done = false;
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;

	do { // use do-while branch out to simplify error handling
		// Create window and add listeners
		if((window = $(window, opengl, createOpenGLWindow)("Kalisko OpenGL test")) == NULL) {
			break;
		}

		$(void, event, attachEventListener)(window, "mouseDown", NULL, &listener_mouseDown);
		$(void, event, attachEventListener)(window, "mouseUp", NULL, &listener_mouseUp);
		$(void, event, attachEventListener)(window, "keyDown", NULL, &listener_keyDown);
		$(void, event, attachEventListener)(window, "keyUp", NULL, &listener_keyUp);
		$(void, event, attachEventListener)(window, "display", NULL, &listener_display);


		// Create geometry
		if((mesh = $(OpenGLMesh *, opengl, createMesh)(3, 1, GL_STATIC_DRAW)) == NULL) {
			break;
		}

		// Draw a simple tri colored triangle
		mesh->vertices[0].position[0] = -1;
		mesh->vertices[0].position[1] = 0;
		mesh->vertices[0].position[2] = 0;
		mesh->vertices[1].position[0] = 1;
		mesh->vertices[1].position[1] = 0;
		mesh->vertices[1].position[2] = 0;
		mesh->vertices[2].position[0] = 0;
		mesh->vertices[2].position[1] = 1;
		mesh->vertices[2].position[2] = 0;
		mesh->vertices[0].normal[0] = 0;
		mesh->vertices[0].normal[1] = 0;
		mesh->vertices[0].normal[2] = -1;
		mesh->vertices[1].normal[0] = 0;
		mesh->vertices[1].normal[1] = 0;
		mesh->vertices[1].normal[2] = -1;
		mesh->vertices[2].normal[0] = 0;
		mesh->vertices[2].normal[1] = 0;
		mesh->vertices[2].normal[2] = -1;
		mesh->vertices[0].color[0] = 1;
		mesh->vertices[0].color[1] = 0;
		mesh->vertices[0].color[2] = 0;
		mesh->vertices[0].color[3] = 1;
		mesh->vertices[1].color[0] = 0;
		mesh->vertices[1].color[1] = 1;
		mesh->vertices[1].color[2] = 0;
		mesh->vertices[1].color[3] = 1;
		mesh->vertices[2].color[0] = 0;
		mesh->vertices[2].color[1] = 0;
		mesh->vertices[2].color[2] = 1;
		mesh->vertices[2].color[3] = 1;
		mesh->triangles[0].indices[0] = 0;
		mesh->triangles[0].indices[1] = 1;
		mesh->triangles[0].indices[2] = 2;

		// Write the mesh changes back to the OpenGL buffer
		if(!$(bool, opengl, updateMesh)(mesh)) {
			break;
		}

		GString *path;

		// Read vertex shader source
		path = g_string_new($$(char *, getExecutablePath)());
		g_string_append(path, "/modules/opengltest/shader.glslv");

		char *vertexShaderSource;
		unsigned long vertexShaderLength;
		if(!g_file_get_contents(path->str, &vertexShaderSource, &vertexShaderLength, NULL)) {
			LOG_ERROR("Failed to read vertex shader source from %s", path->str);
			g_string_free(path, true);
			break;
		}

		g_string_free(path, true);

		// Read fragment shader source
		path = g_string_new($$(char *, getExecutablePath)());
		g_string_append(path, "/modules/opengltest/shader.glslf");

		char *fragmentShaderSource;
		unsigned long fragmentShaderLength;
		if(!g_file_get_contents(path->str, &fragmentShaderSource, &fragmentShaderLength, NULL)) {
			LOG_ERROR("Failed to read fragment shader source from %s", path->str);
			g_string_free(path, true);
			break;
		}

		g_string_free(path, true);

		// Compile and link shader program
		if((vertexShader = $(GLuint, opengl, createShaderFromString)(vertexShaderSource, GL_VERTEX_SHADER)) == 0) {
			break;
		}

		if((fragmentShader = $(GLuint, opengl, createShaderFromString)(fragmentShaderSource, GL_FRAGMENT_SHADER)) == 0) {
			break;
		}

		if((program = $(GLuint, opengl, createShaderProgram)(vertexShader, fragmentShader, true)) == 0) {
			break;
		}

		vertexShader = 0;
		fragmentShader = 0;

		// Create material and attach shader program
		if(!$(bool, opengl, createOpenGLMaterial("opengltest"))) {
			break;
		}

		if(!$(bool, opengl, attachOpenGLMaterialShaderProgram("opengltest", program))) {
			break;
		}

		done = true;
	} while(false);

	if(!done) {
		if(vertexShader != 0) {
			glDeleteShader(vertexShader);
		}

		if(fragmentShader != 0) {
			glDeleteShader(fragmentShader);
		}

		if(program != 0) {
			glDeleteProgram(program);
		}

		if(mesh != NULL) {
			$(void, opengl, freeMesh)(mesh);
		}

		if(window != NULL) {
			$(void, opengl, freeOpenGLWindow)(window);
		}

		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, opengl, deleteOpenGLMaterial)("opengltest");
	$(void, opengl, freeMesh)(mesh);

	$(void, event, detachEventListener)(window, "mouseDown", NULL, &listener_mouseDown);
	$(void, event, detachEventListener)(window, "mouseUp", NULL, &listener_mouseUp);
	$(void, event, detachEventListener)(window, "keyDown", NULL, &listener_keyDown);
	$(void, event, detachEventListener)(window, "keyUp", NULL, &listener_keyUp);
	$(void, event, detachEventListener)(window, "display", NULL, &listener_display);
	$(void, opengl, freeOpenGLWindow)(window);
}

static void listener_mouseDown(void *subject, const char *event, void *data, va_list args)
{
	int button = va_arg(args, int);
	int x = va_arg(args, int);
	int y = va_arg(args, int);

	LOG_DEBUG("Mouse button %d down at %d/%d", button, x, y);
}

static void listener_mouseUp(void *subject, const char *event, void *data, va_list args)
{
	int button = va_arg(args, int);
	int x = va_arg(args, int);
	int y = va_arg(args, int);

	LOG_DEBUG("Mouse button %d up at %d/%d", button, x, y);
}

static void listener_keyDown(void *subject, const char *event, void *data, va_list args)
{
	int key = va_arg(args, int);
	int x = va_arg(args, int);
	int y = va_arg(args, int);

	LOG_DEBUG("Key '%c' down at %d/%d", key, x, y);

	switch(key) {
		case 27: // escape
			safeRevokeModule("opengltest");
		break;
		case 'f':
			glutFullScreenToggle();
		break;
	}
}

static void listener_keyUp(void *subject, const char *event, void *data, va_list args)
{
	int key = va_arg(args, int);
	int x = va_arg(args, int);
	int y = va_arg(args, int);

	LOG_DEBUG("Key '%c' up at %d/%d", key, x, y);
}

static void listener_display(void *subject, const char *event, void *data, va_list args)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

