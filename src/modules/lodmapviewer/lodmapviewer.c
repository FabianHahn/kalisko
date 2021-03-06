/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "modules/lodmap/lodmap.h"
#include "modules/lodmap/imagesource.h"
#include "modules/lodmap/importsource.h"
#include "modules/lodmap/export.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/config/config.h"
#include "modules/image/image.h"
#include "modules/image/io.h"
#define API

MODULE_NAME("lodmapviewer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Viewer application for LOD maps");
MODULE_VERSION(0, 4, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("freeglut", 0, 1, 0), MODULE_DEPENDENCY("opengl", 0, 29, 6), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("lodmap", 0, 15, 3), MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("config", 0, 4, 2), MODULE_DEPENDENCY("image", 0, 5, 20), MODULE_DEPENDENCY("image_pnm", 0, 1, 9), MODULE_DEPENDENCY("image_png", 0, 1, 5));

static FreeglutWindow *window = NULL;
static OpenGLCamera *camera = NULL;
static Matrix *perspectiveMatrix = NULL;
static bool keysPressed[256];
static int currentWidth = 800;
static int currentHeight = 600;
static bool cameraTiltEnabled = false;
static OpenGLLodMap *lodmap;
static bool autoUpdate = true;
static bool autoExpand = true;
static bool autoMove = false;
static bool recording = false;
static unsigned int recordFrame = 0;
static Vector *lightPosition;
static Vector *lightColor;

static void listener_keyDown(void *subject, const char *event, void *data, va_list args);
static void listener_keyUp(void *subject, const char *event, void *data, va_list args);
static void listener_display(void *subject, const char *event, void *data, va_list args);
static void listener_update(void *subject, const char *event, void *data, va_list args);
static void listener_reshape(void *subject, const char *event, void *data, va_list args);
static void listener_mouseMove(void *subject, const char *event, void *data, va_list args);
static void listener_close(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	// Create window and add listeners
	if((window = createFreeglutWindow("Kalisko LOD map viewer")) == NULL) {
		freeFreeglutWindow(window);
		return false;
	}

	glutReshapeWindow(currentWidth, currentHeight);
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(currentWidth / 2, currentHeight / 2);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	Store *config = getConfig();
	lodmap = createOpenGLLodMapFromStore(config);
	if(lodmap == NULL) {
		freeFreeglutWindow(window);
		return false;
	}

	camera = createOpenGLCamera();
	float *cameraData = getVectorData(camera->position);
	cameraData[0] = 0.5f;
	cameraData[1] = 0.5f;
	cameraData[2] = 0.5f;
	updateOpenGLCameraLookAtMatrix(camera);
	activateOpenGLCamera(camera);
		
	lookupQuadtreeNode(lodmap->quadtree, 12.0, 12.0, 0);
	updateOpenGLLodMap(lodmap, camera->position, autoExpand);

	perspectiveMatrix = createPerspectiveMatrix(2.0 * G_PI * 10.0 / 360.0, (double) currentWidth / currentHeight, 0.1, 100.0);
	OpenGLUniform *perspectiveUniform = createOpenGLUniformMatrix(perspectiveMatrix);
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "perspective");
	attachOpenGLUniform(getOpenGLGlobalUniforms(), "perspective", perspectiveUniform);
	lightPosition = createVector3(-10.0, 100.0, -50.0);
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "lightPosition");
	attachOpenGLUniform(getOpenGLGlobalUniforms(), "lightPosition", createOpenGLUniformVector(lightPosition));
	lightColor = createVector4(1.0, 1.0, 1.0, 1.0);
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "lightColor");
	attachOpenGLUniform(getOpenGLGlobalUniforms(), "lightColor", createOpenGLUniformVector(lightColor));
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "ambient");
	attachOpenGLUniform(getOpenGLGlobalUniforms(), "ambient", createOpenGLUniformFloat(0.25));
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "specular");
	attachOpenGLUniform(getOpenGLGlobalUniforms(), "specular", createOpenGLUniformFloat(0.4));


	attachEventListener(window, "keyDown", NULL, &listener_keyDown);
	attachEventListener(window, "keyUp", NULL, &listener_keyUp);
	attachEventListener(window, "display", NULL, &listener_display);
	attachEventListener(window, "update", NULL, &listener_update);
	attachEventListener(window, "reshape", NULL, &listener_reshape);
	attachEventListener(window, "passiveMouseMove", NULL, &listener_mouseMove);
	attachEventListener(window, "mouseMove", NULL, &listener_mouseMove);
	attachEventListener(window, "close", NULL, &listener_close);

	glutWarpPointer(currentWidth / 2, currentHeight / 2);

	return true;
}

MODULE_FINALIZE
{
	detachEventListener(window, "keyDown", NULL, &listener_keyDown);
	detachEventListener(window, "keyUp", NULL, &listener_keyUp);
	detachEventListener(window, "display", NULL, &listener_display);
	detachEventListener(window, "update", NULL, &listener_update);
	detachEventListener(window, "reshape", NULL, &listener_reshape);
	detachEventListener(window, "passiveMouseMove", NULL, &listener_mouseMove);
	detachEventListener(window, "mouseMove", NULL, &listener_mouseMove);
	detachEventListener(window, "close", NULL, &listener_close);
	freeFreeglutWindow(window);

	freeOpenGLLodMap(lodmap);
	freeOpenGLCamera(camera);
	freeMatrix(perspectiveMatrix);

	detachOpenGLUniform(getOpenGLGlobalUniforms(), "perspective");
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "lightPosition");
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "lightColor");
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "ambient");
	detachOpenGLUniform(getOpenGLGlobalUniforms(), "specular");

	freeVector(lightPosition);
	freeVector(lightColor);
}

static void listener_keyDown(void *subject, const char *event, void *data, va_list args)
{
	int key = va_arg(args, int);

	if(key >= 0 && key < 256) {
		keysPressed[key] = true;
	}

	switch(key) {
		case 27: // escape
			safeRevokeModule("lodmapviewer");
		break;
		case 'f':
			glutFullScreenToggle();
		break;
		case 'p':
			if(lodmap->polygonMode == GL_FILL) {
				lodmap->polygonMode = GL_LINE;
				logNotice("Set polygon rendering mode to 'GL_LINE'");
			} else {
				lodmap->polygonMode = GL_FILL;
				logNotice("Set polygon rendering mode to 'GL_FILL'");
			}

			updateOpenGLLodMap(lodmap, camera->position, autoExpand);
		break;
		case 'u':
			autoUpdate = !autoUpdate;
			logNotice("%s automatic LOD map updates", autoUpdate ? "Enabled" : "Disabled");

			if(autoUpdate) {
				updateOpenGLLodMap(lodmap, camera->position, autoExpand);
			}
		break;
		case 'x':
			autoExpand = !autoExpand;
			logNotice("%s automatic LOD map expansion", autoExpand ? "Enabled" : "Disabled");
		break;
		case 't':
			{
				Image *screenshot = getOpenGLScreenshot(0, 0, currentWidth, currentHeight);
				debugImage(screenshot);
				freeImage(screenshot);
			}
		break;
		case 'r':
			recording = !recording;
		break;
		case 'm':
			autoMove = !autoMove;
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
	drawOpenGLLodMap(lodmap);

	if(recording) {
		Image *screenshot = getOpenGLScreenshot(0, 0, currentWidth, currentHeight);
		GString *name = g_string_new("record");
		g_string_append_printf(name, "%04d.ppm", recordFrame++);
		writeImageToFile(screenshot, name->str);
		g_string_free(name, true);
		freeImage(screenshot);
	}
}

static void listener_update(void *subject, const char *event, void *data, va_list args)
{
	double dt = va_arg(args, double);
	bool cameraChanged = false;

	if(keysPressed['w']) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_FORWARD, dt);
		cameraChanged = true;
	}

	if(keysPressed['a']) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_LEFT, dt);
		cameraChanged = true;
	}

	if(keysPressed['s']) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_BACK, dt);
		cameraChanged = true;
	}

	if(keysPressed['d']) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_RIGHT, dt);
		cameraChanged = true;
	}

	if(keysPressed[' ']) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_UP, dt);
		cameraChanged = true;
	}

	if(keysPressed['c']) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_DOWN, dt);
		cameraChanged = true;
	}

	if(autoMove) {
		moveOpenGLCamera(camera, OPENGL_CAMERA_MOVE_FORWARD, 1e-2);
		cameraChanged = true;
	}

	// We need to update the camera matrix if some movement happened
	if(cameraChanged) {
		updateOpenGLCameraLookAtMatrix(camera);

		if(autoUpdate) {
			updateOpenGLLodMap(lodmap, camera->position, autoExpand);
		}
	}

	glutPostRedisplay();
}

static void listener_reshape(void *subject, const char *event, void *data, va_list args)
{
	int w = va_arg(args, int);
	int h = va_arg(args, int);

	glViewport(0, 0, w, h);
	Matrix *newPerspectiveMatrix = createPerspectiveMatrix(2.0 * G_PI * 50.0 / 360.0, (double) w / h, 0.1, 100.0);
	assignMatrix(perspectiveMatrix, newPerspectiveMatrix);
	freeMatrix(newPerspectiveMatrix);

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
		tiltOpenGLCamera(camera, OPENGL_CAMERA_TILT_LEFT, 0.005 * dx);
		cameraChanged = true;
	}

	if(dy != 0) {
		tiltOpenGLCamera(camera, OPENGL_CAMERA_TILT_UP, 0.005 * dy);
		cameraChanged = true;
	}

	// We need to update the camera matrix if some tilting happened
	if(cameraChanged) {
		updateOpenGLCameraLookAtMatrix(camera);
		glutPostRedisplay();
		glutWarpPointer(cx, cy);
	}
}

static void listener_close(void *subject, const char *event, void *data, va_list args)
{
	safeRevokeModule("lodmapviewer");
}
