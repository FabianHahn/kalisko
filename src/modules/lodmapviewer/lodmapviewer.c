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
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/config/config.h"
#include "api.h"

MODULE_NAME("lodmapviewer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Viewer application for LOD maps");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("freeglut", 0, 1, 0), MODULE_DEPENDENCY("opengl", 0, 29, 0), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("lodmap", 0, 4, 5), MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("config", 0, 4, 2));

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
	if((window = $(FreeglutWindow *, freeglut, createFreeglutWindow)("Kalisko LOD map viewer")) == NULL) {
		$(void, freeglut, freeFreeglutWindow)(window);
		return false;
	}

	glutReshapeWindow(800, 600);
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(400, 300);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	Store *config = $(Store *, config, getConfig)();
	Store *configLodMapPath;
	char *lodMapPath = "/tmp/kaliskomap/";
	if((configLodMapPath = $(Store *, store, getStorePath)(config, "lodmap/path")) != NULL && configLodMapPath->type == STORE_STRING) {
		lodMapPath = configLodMapPath->content.string;
	} else {
		LOG_INFO("lodmapviewer config parameter 'lodmap/path' not found, defaulting to '%s'", lodMapPath);
	}

	Store *configLodMapPrefix;
	char *lodMapPrefix = "map";
	if((configLodMapPrefix = $(Store *, store, getStorePath)(config, "lodmap/prefix")) != NULL && configLodMapPrefix->type == STORE_STRING) {
		lodMapPrefix = configLodMapPrefix->content.string;
	} else {
		LOG_INFO("lodmapviewer config parameter 'lodmap/prefix' not found, defaulting to '%s'", lodMapPrefix);
	}

	Store *configLodMapExtension;
	char *lodMapExtension = "png";
	if((configLodMapExtension = $(Store *, store, getStorePath)(config, "lodmap/extension")) != NULL && configLodMapExtension->type == STORE_STRING) {
		lodMapExtension = configLodMapExtension->content.string;
	} else {
		LOG_INFO("lodmapviewer config parameter 'lodmap/extension' not found, defaulting to '%s'", lodMapExtension);
	}

	GString *dataPrefix = g_string_new(lodMapPath);
	g_string_append_printf(dataPrefix, "/%s", lodMapPrefix);
	lodmap = $(OpenGLLodMap *, lodmap, createOpenGLLodMap)(1.5, 2, 128, dataPrefix->str, lodMapExtension);
	g_string_free(dataPrefix, true);

	if(lodmap == NULL) {
		$(void, freeglut, freeFreeglutWindow)(window);
		return false;
	}
		
	$(QuadtreeNode *, quadtree, lookupQuadtreeNodeWorld)(lodmap->quadtree, 3.0, 3.0, 0);

	camera = $(OpenGLCamera *, opengl, createOpenGLCamera)();
	float *cameraData = $(float *, linalg, getVectorData)(camera->position);
	cameraData[0] = 0.5f;
	cameraData[1] = 0.5f;
	cameraData[2] = 0.5f;
	$(void, opengl, activateOpenGLCamera)(camera);
	$(void, lodmap, updateOpenGLLodMap)(lodmap, camera->position);

	perspectiveMatrix = $(Matrix *, linalg, createPerspectiveMatrix)(2.0 * G_PI * 10.0 / 360.0, (double) 800 / 600, 0.1, 100.0);
	OpenGLUniform *perspectiveUniform = $(OpenGLUniform *, opengl, createOpenGLUniformMatrix)(perspectiveMatrix);
	$(bool, opengl, attachOpenGLUniform)($(OpenGLUniformAttachment *, opengl, getOpenGLGlobalUniforms)(), "perspective", perspectiveUniform);

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

	$(void, lodmap, freeOpenGLLodMap)(lodmap);
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
			$(void, module_util, safeRevokeModule)("lodmapviewer");
		break;
		case 'f':
			glutFullScreenToggle();
		break;
		case 'p':
			if(lodmap->polygonMode == GL_FILL) {
				lodmap->polygonMode = GL_LINE;
				LOG_INFO("Set polygon rendering mode to 'GL_LINE'");
			} else {
				lodmap->polygonMode = GL_FILL;
				LOG_INFO("Set polygon rendering mode to 'GL_FILL'");
			}

			$(void, lodmap, updateOpenGLLodMap)(lodmap, camera->position, autoExpand);
		break;
		case 'u':
			autoUpdate = !autoUpdate;
			LOG_INFO("%s automatic LOD map updates", autoUpdate ? "Enabled" : "Disabled");

			if(autoUpdate) {
				$(void, lodmap, updateOpenGLLodMap)(lodmap, camera->position, autoExpand);
			}
		break;
		case 'x':
			autoExpand = !autoExpand;
			LOG_INFO("%s automatic LOD map expansion", autoExpand ? "Enabled" : "Disabled");
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
	$(void, lodmap, drawOpenGLLodMap)(lodmap);
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

		if(autoUpdate) {
			$(void, lodmap, updateOpenGLLodMap)(lodmap, camera->position, autoExpand);
		}
	}

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
	$(void, module_util, safeRevokeModule)("lodmapviewer");
}
