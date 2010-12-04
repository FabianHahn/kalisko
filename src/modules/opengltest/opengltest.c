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
#include "modules/freeglut/freeglut.h"
#include "modules/opengl/opengl.h"
#include "modules/opengl/material.h"
#include "modules/opengl/shader.h"
#include "modules/opengl/mesh.h"
#include "modules/opengl/transform.h"
#include "modules/opengl/camera.h"
#include "modules/module_util/module_util.h"
#include "modules/event/event.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"

#include "api.h"

MODULE_NAME("opengltest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The opengltest module creates a simple OpenGL window sample");
MODULE_VERSION(0, 7, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("freeglut", 0, 1, 0), MODULE_DEPENDENCY("opengl", 0, 8, 3), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2), MODULE_DEPENDENCY("linalg", 0, 1, 13));

static FreeglutWindow *window = NULL;
static OpenGLMesh *mesh = NULL;
static GLuint program = 0;
static OpenGLCamera *camera = NULL;
static Matrix *cameraMatrix = NULL;
static Matrix *perspectiveMatrix = NULL;
static bool keysPressed[256];
static int currentWidth = 800;
static int currentHeight = 600;
static bool cameraTiltEnabled = false;

static void listener_mouseDown(void *subject, const char *event, void *data, va_list args);
static void listener_mouseUp(void *subject, const char *event, void *data, va_list args);
static void listener_keyDown(void *subject, const char *event, void *data, va_list args);
static void listener_keyUp(void *subject, const char *event, void *data, va_list args);
static void listener_display(void *subject, const char *event, void *data, va_list args);
static void listener_update(void *subject, const char *event, void *data, va_list args);
static void listener_reshape(void *subject, const char *event, void *data, va_list args);
static void listener_mouseMove(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	$$(void, breakpoint)();

	bool done = false;
	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;

	do { // use do-while branch out to simplify error handling
		// Create window and add listeners
		if((window = $(FreeglutWindow *, opengl, createFreeglutWindow)("Kalisko OpenGL test")) == NULL) {
			break;
		}

		glutReshapeWindow(800, 600);
		glutSetCursor(GLUT_CURSOR_NONE);
		glutWarpPointer(400, 300);

		// Create geometry
		if((mesh = $(OpenGLMesh *, opengl, createOpenGLMesh)(4, 4, GL_STATIC_DRAW)) == NULL) {
			break;
		}

		// Draw a simple tri colored triangle
		mesh->vertices[0].position[0] = -0.5;
		mesh->vertices[0].position[1] = -0.25;
		mesh->vertices[0].position[2] = 1.5;
		mesh->vertices[1].position[0] = 0.5;
		mesh->vertices[1].position[1] = -0.25;
		mesh->vertices[1].position[2] = 1.5;
		mesh->vertices[2].position[0] = 0;
		mesh->vertices[2].position[1] = 0.566;
		mesh->vertices[2].position[2] = 1.772;
		mesh->vertices[3].position[0] = 0;
		mesh->vertices[3].position[1] = -0.25;
		mesh->vertices[3].position[2] = 2.366;
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
		mesh->vertices[3].color[0] = 1;
		mesh->vertices[3].color[1] = 1;
		mesh->vertices[3].color[2] = 1;
		mesh->vertices[3].color[3] = 1;
		mesh->triangles[0].indices[0] = 0;
		mesh->triangles[0].indices[1] = 1;
		mesh->triangles[0].indices[2] = 2;
		mesh->triangles[1].indices[0] = 0;
		mesh->triangles[1].indices[1] = 1;
		mesh->triangles[1].indices[2] = 3;
		mesh->triangles[2].indices[0] = 0;
		mesh->triangles[2].indices[1] = 2;
		mesh->triangles[2].indices[2] = 3;
		mesh->triangles[3].indices[0] = 1;
		mesh->triangles[3].indices[1] = 2;
		mesh->triangles[3].indices[2] = 3;

		glEnable(GL_DEPTH_TEST);

		// Write the mesh changes back to the OpenGL buffer
		if(!$(bool, opengl, updateOpenGLMesh)(mesh)) {
			break;
		}

		// Read and compile vertex shader source
		char *execpath = $$(char *, getExecutablePath)();
		GString *vertexShaderFile = g_string_new(execpath);
		g_string_append(vertexShaderFile, "/modules/opengltest/shader.glslv");

		if((vertexShader = $(GLuint, opengl, createOpenGLShaderFromFile)(vertexShaderFile->str, GL_VERTEX_SHADER)) == 0) {
			break;
		}

		g_string_free(vertexShaderFile, true);

		// Read and compile fragment shader source
		GString *fragmentShaderFile = g_string_new(execpath);
		g_string_append(fragmentShaderFile, "/modules/opengltest/shader.glslf");

		if((fragmentShader = $(GLuint, opengl, createOpenGLShaderFromFile)(fragmentShaderFile->str, GL_FRAGMENT_SHADER)) == 0) {
			break;
		}

		if((program = $(GLuint, opengl, createOpenGLShaderProgram)(vertexShader, fragmentShader, true)) == 0) {
			break;
		}

		g_string_free(fragmentShaderFile, true);
		free(execpath);

		vertexShader = 0;
		fragmentShader = 0;

		// Create material and attach shader program
		if(!$(bool, opengl, createOpenGLMaterial)("opengltest")) {
			break;
		}

		camera = $(OpenGLCamera *, opengl, createOpenGLCamera)();
		cameraMatrix = $(Matrix *, opengl, getOpenGLCameraLookAtMatrix)(camera);
		perspectiveMatrix = $(Matrix *, opengl, createPerspectiveMatrix)(2.0 * G_PI * 50.0 / 360.0, 1.0, 0.1, 100.0);
		
		OpenGLUniform *cameraUniform = $(OpenGLUniform *, opengl, createOpenGLUniformMatrix)(cameraMatrix);
		OpenGLUniform *perspectiveUniform = $(OpenGLUniform *, opengl, createOpenGLUniformMatrix)(perspectiveMatrix);

		if(!$(bool, opengl, attachOpenGLMaterialShaderProgram)("opengltest", program)) {
			break;
		}

		if(!$(bool, opengl, attachOpenGLMaterialUniform)("opengltest", "camera", cameraUniform)) {
			break;
		}

		if(!$(bool, opengl, attachOpenGLMaterialUniform)("opengltest", "perspective", perspectiveUniform)) {
			break;
		}

		done = true;
	} while(false);

	if(!done) {
		if(camera != NULL) {
			$(void, linalg, freeOpenGLCamera)(camera);
		}

		if(cameraMatrix != NULL) {
			$(void, linalg, freeMatrix)(cameraMatrix);
		}

		if(perspectiveMatrix != NULL) {
			$(void, linalg, freeMatrix)(perspectiveMatrix);
		}

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
			$(void, opengl, freeOpenGLMesh)(mesh);
		}

		if(window != NULL) {
			$(void, opengl, freeFreeglutWindow)(window);
		}

		return false;
	}

	$(void, event, attachEventListener)(window, "mouseDown", NULL, &listener_mouseDown);
	$(void, event, attachEventListener)(window, "mouseUp", NULL, &listener_mouseUp);
	$(void, event, attachEventListener)(window, "keyDown", NULL, &listener_keyDown);
	$(void, event, attachEventListener)(window, "keyUp", NULL, &listener_keyUp);
	$(void, event, attachEventListener)(window, "display", NULL, &listener_display);
	$(void, event, attachEventListener)(window, "update", NULL, &listener_update);
	$(void, event, attachEventListener)(window, "reshape", NULL, &listener_reshape);
	$(void, event, attachEventListener)(window, "passiveMouseMove", NULL, &listener_mouseMove);
	$(void, event, attachEventListener)(window, "mouseMove", NULL, &listener_mouseMove);

	return true;
}

MODULE_FINALIZE
{
	$(bool, opengl, deleteOpenGLMaterial)("opengltest");
	$(void, opengl, freeOpenGLMesh)(mesh);

	$(void, event, detachEventListener)(window, "mouseDown", NULL, &listener_mouseDown);
	$(void, event, detachEventListener)(window, "mouseUp", NULL, &listener_mouseUp);
	$(void, event, detachEventListener)(window, "keyDown", NULL, &listener_keyDown);
	$(void, event, detachEventListener)(window, "keyUp", NULL, &listener_keyUp);
	$(void, event, detachEventListener)(window, "display", NULL, &listener_display);
	$(void, event, detachEventListener)(window, "update", NULL, &listener_update);
	$(void, event, detachEventListener)(window, "reshape", NULL, &listener_reshape);
	$(void, event, detachEventListener)(window, "passiveMouseMove", NULL, &listener_mouseMove);
	$(void, event, detachEventListener)(window, "mouseMove", NULL, &listener_mouseMove);
	$(void, opengl, freeFreeglutWindow)(window);

	$(void, opengl, freeOpenGLCamera)(camera);
	$(void, linalg, freeMatrix)(cameraMatrix);
	$(void, linalg, freeMatrix)(perspectiveMatrix);
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

	if(key >= 0 && key < 256) {
		keysPressed[key] = true;
	}

	switch(key) {
		case 27: // escape
			$(void, module_util, safeRevokeModule)("opengltest");
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

	if(key >= 0 && key < 256) {
		keysPressed[key] = false;
	}
}

static void listener_display(void *subject, const char *event, void *data, va_list args)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	$(bool, opengl, useOpenGLMaterial)("opengltest");
	$(bool, opengl, drawOpenGLMesh)(mesh);
}

static void listener_update(void *subject, const char *event, void *data, va_list args)
{
	double dt = va_arg(args, double);
	bool cameraChanged = false;

	if(keysPressed['w']) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_FORWARD, dt);
		cameraChanged = true;
	}

	if(keysPressed['a']) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_LEFT, dt);
		cameraChanged = true;
	}

	if(keysPressed['s']) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_BACK, dt);
		cameraChanged = true;
	}

	if(keysPressed['d']) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_RIGHT, dt);
		cameraChanged = true;
	}

	// We need to update the camera matrix if some movement happened
	if(cameraChanged) {
		Matrix *newCameraMatrix = $(Matrix *, opengl, getOpenGLCameraLookAtMatrix)(camera);
		$(void, linalg, assignMatrix)(cameraMatrix, newCameraMatrix);
		$(void, linalg, freeMatrix)(newCameraMatrix);
		glutPostRedisplay();
	}
}

static void listener_reshape(void *subject, const char *event, void *data, va_list args)
{
	int w = va_arg(args, int);
	int h = va_arg(args, int);

	glViewport(0, 0, w, h);
	Matrix *newPerspectiveMatrix = $(Matrix *, opengl, createPerspectiveMatrix)(2.0 * G_PI * 50.0 / 360.0, (double) w / h, 0.1, 100.0);
	$(void, linalg, assignMatrix)(perspectiveMatrix, newPerspectiveMatrix);
	$(void, linalg, freeMatrix)(newPerspectiveMatrix);

	currentWidth = w;
	currentHeight = h;
	glutWarpPointer(w / 2, h / 2);
}

static void listener_mouseMove(void *subject, const char *event, void *data, va_list args)
{
	bool cameraChanged = false;

	int x = va_arg(args, int);
	int y = va_arg(args, int);

	int cx = currentWidth / 2;
	int cy = currentHeight / 2;

	if(!cameraTiltEnabled) {
		if(x == cx && y == cy) {
			cameraTiltEnabled = true;
		}

		return;
	}

	int dx = x - cx;
	int dy = y - cy;

	if(dx != 0) {
		$(void, opengl, tiltOpenGLCamera)(camera, OPENGL_CAMERA_TILT_LEFT, 0.005 * dx);
		cameraChanged = true;
	}

	if(dy != 0) {
		$(void, opengl, tiltOpenGLCamera)(camera, OPENGL_CAMERA_TILT_UP, 0.005 * dy);
		cameraChanged = true;
	}

	// We need to update the camera matrix if some tilting happened
	if(cameraChanged) {
		GString *dump = $(GString *, linalg, dumpVector)(camera->up);
		g_string_free(dump, true);

		Matrix *newCameraMatrix = $(Matrix *, opengl, getOpenGLCameraLookAtMatrix)(camera);
		$(void, linalg, assignMatrix)(cameraMatrix, newCameraMatrix);
		$(void, linalg, freeMatrix)(newCameraMatrix);
		glutPostRedisplay();
		glutWarpPointer(cx, cy);
	}
}
