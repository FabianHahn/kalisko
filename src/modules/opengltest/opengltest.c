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
MODULE_VERSION(0, 2, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 3, 0), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2));

static OpenGLWindow *window;

static void listener_mouseDown(void *subject, const char *event, void *data, va_list args);
static void listener_mouseUp(void *subject, const char *event, void *data, va_list args);
static void listener_keyDown(void *subject, const char *event, void *data, va_list args);
static void listener_keyUp(void *subject, const char *event, void *data, va_list args);
static void listener_display(void *subject, const char *event, void *data, va_list args);

static OpenGLMesh *mesh;

MODULE_INIT
{
	if((window = $(window, opengl, createOpenGLWindow)("Kalisko OpenGL test")) == NULL) {
		return false;
	}

	$(void, event, attachEventListener)(window, "mouseDown", NULL, &listener_mouseDown);
	$(void, event, attachEventListener)(window, "mouseUp", NULL, &listener_mouseUp);
	$(void, event, attachEventListener)(window, "keyDown", NULL, &listener_keyDown);
	$(void, event, attachEventListener)(window, "keyUp", NULL, &listener_keyUp);
	$(void, event, attachEventListener)(window, "display", NULL, &listener_display);

	if((mesh = $(OpenGLMesh *, opengl, createMesh)(3, 1, GL_STATIC_DRAW)) == NULL) {
		return false;
	}

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

	if(!$(bool, opengl, updateMesh)(mesh)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
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

