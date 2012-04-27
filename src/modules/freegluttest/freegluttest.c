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
#include "modules/opengl/camera.h"
#include "modules/opengl/model.h"
#include "modules/module_util/module_util.h"
#include "modules/event/event.h"
#include "modules/linalg/Vector.h"
#include "modules/linalg/Matrix.h"
#include "modules/linalg/transform.h"
#include "modules/mesh/io.h"
#include "modules/scene/scene.h"
#include "modules/particle/particle.h"
#define API

MODULE_NAME("freegluttest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The freegluttest module creates a simple OpenGL window sample using freeglut");
MODULE_VERSION(0, 15, 5);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("freeglut", 0, 1, 0), MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("scene", 0, 8, 0), MODULE_DEPENDENCY("image_png", 0, 1, 2), MODULE_DEPENDENCY("mesh_opengl", 0, 2, 0), MODULE_DEPENDENCY("particle", 0, 6, 6), MODULE_DEPENDENCY("heightmap", 0, 4, 4), MODULE_DEPENDENCY("landscape", 0, 2, 0), MODULE_DEPENDENCY("imagesynth_scene", 0, 1, 0));

static Scene *scene = NULL;
static FreeglutWindow *window = NULL;
static OpenGLCamera *camera = NULL;
static Matrix *perspectiveMatrix = NULL;
static bool keysPressed[256];
static int currentWidth = 800;
static int currentHeight = 600;
static bool cameraTiltEnabled = false;
static float rotation = 0.0f;

static void listener_keyDown(void *subject, const char *event, void *data, va_list args);
static void listener_keyUp(void *subject, const char *event, void *data, va_list args);
static void listener_display(void *subject, const char *event, void *data, va_list args);
static void listener_update(void *subject, const char *event, void *data, va_list args);
static void listener_reshape(void *subject, const char *event, void *data, va_list args);
static void listener_mouseMove(void *subject, const char *event, void *data, va_list args);
static void listener_close(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	bool done = false;
	char *execpath = NULL;

	do { // use do-while branch out to simplify error handling
		// Create window and add listeners
		if((window = $(FreeglutWindow *, freeglut, createFreeglutWindow)("Kalisko freeglut OpenGL test")) == NULL) {
			break;
		}

		glutReshapeWindow(800, 600);
		glutSetCursor(GLUT_CURSOR_NONE);
		glutWarpPointer(400, 300);

		execpath = $$(char *, getExecutablePath)();

		GString *scenePath = g_string_new(execpath);
		g_string_append(scenePath, "/modules/freegluttest/scene.store");
		scene = $(Scene *, scene, createScene)(scenePath->str, execpath);
		g_string_free(scenePath, true);

		if(scene == NULL) {
			break;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);

		camera = $(OpenGLCamera *, opengl, createOpenGLCamera)();
		$(void, opengl, activateOpenGLCamera)(camera);

		perspectiveMatrix = $(Matrix *, linalg, createPerspectiveMatrix)(2.0 * G_PI * 10.0 / 360.0, (double) 800 / 600, 0.1, 100.0);
		OpenGLUniform *perspectiveUniform = $(OpenGLUniform *, opengl, createOpenGLUniformMatrix)(perspectiveMatrix);
		$(bool, opengl, attachOpenGLUniform)($(OpenGLUniformAttachment *, opengl, getOpenGLGlobalUniforms)(), "perspective", perspectiveUniform);
		
		OpenGLModel *model = g_hash_table_lookup(scene->models, "particles");
		if(model != NULL) {
			OpenGLParticles *particles = $(OpenGLParticles *, particle, getOpenGLParticles)(model->primitive);
			particles->properties.aspectRatio = 800.0f / 600.0f;
		}

		done = true;
	} while(false);

	if(execpath != NULL) {
		free(execpath);
	}

	if(!done) {
		if(scene != NULL) {
			$(void, scene, freeScene)(scene);
		}

		if(camera != NULL) {
			$(void, linalg, freeOpenGLCamera)(camera);
		}

		if(window != NULL) {
			$(void, freeglut, freeFreeglutWindow)(window);
		}

		if(perspectiveMatrix != NULL) {
			$(void, linalg, freeMatrix)(perspectiveMatrix);
		}

		return false;
	}

	$(void, event, attachEventListener)(window, "keyDown", NULL, &listener_keyDown);
	$(void, event, attachEventListener)(window, "keyUp", NULL, &listener_keyUp);
	$(void, event, attachEventListener)(window, "display", NULL, &listener_display);
	$(void, event, attachEventListener)(window, "update", NULL, &listener_update);
	$(void, event, attachEventListener)(window, "reshape", NULL, &listener_reshape);
	$(void, event, attachEventListener)(window, "passiveMouseMove", NULL, &listener_mouseMove);
	$(void, event, attachEventListener)(window, "mouseMove", NULL, &listener_mouseMove);
	$(void, event, attachEventListener)(window, "close", NULL, &listener_close);

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(window, "keyDown", NULL, &listener_keyDown);
	$(void, event, detachEventListener)(window, "keyUp", NULL, &listener_keyUp);
	$(void, event, detachEventListener)(window, "display", NULL, &listener_display);
	$(void, event, detachEventListener)(window, "update", NULL, &listener_update);
	$(void, event, detachEventListener)(window, "reshape", NULL, &listener_reshape);
	$(void, event, detachEventListener)(window, "passiveMouseMove", NULL, &listener_mouseMove);
	$(void, event, detachEventListener)(window, "mouseMove", NULL, &listener_mouseMove);
	$(void, event, detachEventListener)(window, "close", NULL, &listener_close);
	$(void, freeglut, freeFreeglutWindow)(window);

	$(void, scene, freeScene)(scene);
	$(void, opengl, freeOpenGLCamera)(camera);
	$(void, linalg, freeMatrix)(perspectiveMatrix);
}

static void listener_keyDown(void *subject, const char *event, void *data, va_list args)
{
	int key = va_arg(args, int);

	if(key >= 0 && key < 256) {
		keysPressed[key] = true;
	}

	switch(key) {
		case 27: // escape
			$(void, module_util, safeRevokeModule)("freegluttest");
		break;
		case 'f':
			glutFullScreenToggle();
		break;
	}
}

static void listener_keyUp(void *subject, const char *event, void *data, va_list args)
{
	int key = va_arg(args, int);

	if(key >= 0 && key < 256) {
		keysPressed[key] = false;
	}
}

static void listener_display(void *subject, const char *event, void *data, va_list args)
{
	glClearColor(0.9f, 0.9f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	$(void, scene, drawScene)(scene);
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

	if(keysPressed[' ']) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_UP, dt);
		cameraChanged = true;
	}

	if(keysPressed['c']) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_DOWN, dt);
		cameraChanged = true;
	}

	// We need to update the camera matrix if some movement happened
	if(cameraChanged) {
		$(void, opengl, updateOpenGLCameraLookAtMatrix)(camera);
	}

	rotation += dt;

	OpenGLModel *model = g_hash_table_lookup(scene->models, "tetrahedron");
	if(model != NULL) {
		model->rotationY = rotation;
		$(void, opengl, updateOpenGLModelTransform)(model);
	}

	$(void, scene, updateScene)(scene, dt);

	glutPostRedisplay();
}

static void listener_reshape(void *subject, const char *event, void *data, va_list args)
{
	int w = va_arg(args, int);
	int h = va_arg(args, int);

	glViewport(0, 0, w, h);
	Matrix *newPerspectiveMatrix = $(Matrix *, linalg, createPerspectiveMatrix)(2.0 * G_PI * 50.0 / 360.0, (double) w / h, 0.1, 100.0);
	$(void, linalg, assignMatrix)(perspectiveMatrix, newPerspectiveMatrix);
	$(void, linalg, freeMatrix)(newPerspectiveMatrix);

	currentWidth = w;
	currentHeight = h;
	glutWarpPointer(w / 2, h / 2);

	OpenGLModel *model = g_hash_table_lookup(scene->models, "particles");
	if(model != NULL) {
		OpenGLParticles *particles = $(OpenGLParticles *, particle, getOpenGLParticles)(model->primitive);
		particles->properties.aspectRatio = (float) w / h;
	}
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
		$(void, opengl, updateOpenGLCameraLookAtMatrix)(camera);
		glutPostRedisplay();
		glutWarpPointer(cx, cy);
	}
}

static void listener_close(void *subject, const char *event, void *data, va_list args)
{
	$(void, module_util, safeRevokeModule)("freegluttest");
}
