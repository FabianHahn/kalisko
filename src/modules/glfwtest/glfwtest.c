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
#include <GL/glfw.h>
#include <glib.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "modules/glfw/module_glfw.h"
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

#include "api.h"

MODULE_NAME("glfwtest");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The glfwtest module creates a simple OpenGL window sample using glfw");
MODULE_VERSION(0, 2, 7);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("glfw", 0, 2, 3), MODULE_DEPENDENCY("opengl", 0, 21, 0), MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("module_util", 0, 1, 2), MODULE_DEPENDENCY("linalg", 0, 3, 3), MODULE_DEPENDENCY("scene", 0, 4, 8), MODULE_DEPENDENCY("image_png", 0, 1, 2), MODULE_DEPENDENCY("mesh_opengl", 0, 2, 0), MODULE_DEPENDENCY("particle", 0, 6, 6), MODULE_DEPENDENCY("heightmap", 0, 1, 0));

static Scene *scene = NULL;
static OpenGLCamera *camera = NULL;
static Matrix *perspectiveMatrix = NULL;
static float rotation = 0.0f;
static int lastx;
static int lasty;

static void listener_display(void *subject, const char *event, void *data, va_list args);
static void listener_update(void *subject, const char *event, void *data, va_list args);
static void listener_reshape(void *subject, const char *event, void *data, va_list args);
static void listener_mouseMove(void *subject, const char *event, void *data, va_list args);
static void listener_close(void *subject, const char *event, void *data, va_list args);

MODULE_INIT
{
	char *execpath = NULL;

	int width = 800;
	int height = 600;
	bool fullscreen = false;

#ifdef GLFWTEST_FULLSCREEN
	GLFWvidmode mode;
	glfwGetDesktopMode(&mode);
	width = mode.Width;
	height = mode.Height;
	fullscreen = true;
#endif

	// Create window and add listeners
	if(!$(bool, glfw, openGlfwWindow)("Kalisko glfw OpenGL test", width, height, fullscreen, true)) {
		return false;
	}

	glfwDisable(GLFW_MOUSE_CURSOR);
	glfwGetMousePos(&lastx, &lasty);

	execpath = $$(char *, getExecutablePath)();

	GString *scenePath = g_string_new(execpath);
	g_string_append(scenePath, "/modules/freegluttest/scene.store");
	scene = $(Scene *, scene, createScene)(scenePath->str, execpath);
	g_string_free(scenePath, true);

	if(scene == NULL) {
		$(bool, glfw, closeGlfwWindow)();
		return false;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	camera = $(OpenGLCamera *, opengl, createOpenGLCamera)();
	$(void, opengl, activateOpenGLCamera)(camera);

	perspectiveMatrix = $(Matrix *, linalg, createPerspectiveMatrix)(2.0 * G_PI * 50.0 / 360.0, (double) 800 / 600, 0.1, 100.0);
	OpenGLUniform *perspectiveUniform = $(OpenGLUniform *, opengl, createOpenGLUniformMatrix)(perspectiveMatrix);
	$(void, opengl, addOpenGLGlobalShaderUniform)("perspective", perspectiveUniform);

	OpenGLPrimitive *primitive = $(OpenGLPrimitive *, opengl, getOpenGLModelPrimitive)("particles");
	if(primitive != NULL) {
		OpenGLParticles *particles = $(OpenGLParticles *, particle, getOpenGLParticles)(primitive);
		particles->properties.aspectRatio = 800.0f / 600.0f;
	}

	GlfwHandle *handle = $(GlfwHandle *, glfw, getGlfwHandle)();
	$(void, event, attachEventListener)(handle, "display", NULL, &listener_display);
	$(void, event, attachEventListener)(handle, "update", NULL, &listener_update);
	$(void, event, attachEventListener)(handle, "reshape", NULL, &listener_reshape);
	$(void, event, attachEventListener)(handle, "mouseMove", NULL, &listener_mouseMove);
	$(void, event, attachEventListener)(handle, "close", NULL, &listener_close);

	return true;
}

MODULE_FINALIZE
{
	GlfwHandle *handle = $(GlfwHandle *, glfw, getGlfwHandle)();
	$(void, event, detachEventListener)(handle, "display", NULL, &listener_display);
	$(void, event, detachEventListener)(handle, "update", NULL, &listener_update);
	$(void, event, detachEventListener)(handle, "reshape", NULL, &listener_reshape);
	$(void, event, detachEventListener)(handle, "mouseMove", NULL, &listener_mouseMove);
	$(void, event, detachEventListener)(handle, "close", NULL, &listener_close);

	$(bool, glfw, closeGlfwWindow)();
	$(void, scene, freeScene)(scene);
	$(void, opengl, freeOpenGLCamera)(camera);
	$(void, linalg, freeMatrix)(perspectiveMatrix);
}

static void listener_display(void *subject, const char *event, void *data, va_list args)
{
	glClearColor(0.9f, 0.9f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	$(void, opengl, drawOpenGLModels)();
}

static void listener_update(void *subject, const char *event, void *data, va_list args)
{
	double dt = va_arg(args, double);
	bool cameraChanged = false;

	if(glfwGetKey(GLFW_KEY_ESC)) {
		$(void, module_util, safeRevokeModule)("glfwtest");
	}

	if(glfwGetKey('W')) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_FORWARD, dt);
		cameraChanged = true;
	}

	if(glfwGetKey('A')) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_LEFT, dt);
		cameraChanged = true;
	}

	if(glfwGetKey('S')) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_BACK, dt);
		cameraChanged = true;
	}

	if(glfwGetKey('D')) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_RIGHT, dt);
		cameraChanged = true;
	}

	if(glfwGetKey(' ')) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_UP, dt);
		cameraChanged = true;
	}

	if(glfwGetKey('C')) {
		$(void, opengl, moveOpenGLCamera)(camera, OPENGL_CAMERA_MOVE_DOWN, dt);
		cameraChanged = true;
	}

	// We need to update the camera matrix if some movement happened
	if(cameraChanged) {
		$(void, opengl, updateOpenGLCameraLookAtMatrix)(camera);
	}

	rotation += dt;
	$(bool, opengl, setOpenGLModelRotationY)("tetrahedron", rotation);

	$(void, opengl, updateOpenGLModels)(dt);
}

static void listener_reshape(void *subject, const char *event, void *data, va_list args)
{
	int w = va_arg(args, int);
	int h = va_arg(args, int);

	glViewport(0, 0, w, h);
	Matrix *newPerspectiveMatrix = $(Matrix *, linalg, createPerspectiveMatrix)(2.0 * G_PI * 50.0 / 360.0, (double) w / h, 0.1, 100.0);
	$(void, linalg, assignMatrix)(perspectiveMatrix, newPerspectiveMatrix);
	$(void, linalg, freeMatrix)(newPerspectiveMatrix);

	OpenGLPrimitive *primitive = $(OpenGLPrimitive *, opengl, getOpenGLModelPrimitive)("particles");
	if(primitive != NULL) {
		OpenGLParticles *particles = $(OpenGLParticles *, particle, getOpenGLParticles)(primitive);
		particles->properties.aspectRatio = (float) w / h;
	}
}

static void listener_mouseMove(void *subject, const char *event, void *data, va_list args)
{
	bool cameraChanged = false;

	int x = va_arg(args, int);
	int y = va_arg(args, int);

	int dx = x - lastx;
	int dy = y - lasty;

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
	}

	lastx = x;
	lasty = y;
}

static void listener_close(void *subject, const char *event, void *data, va_list args)
{
	$(void, module_util, safeRevokeModule)("glfwtest");
}

