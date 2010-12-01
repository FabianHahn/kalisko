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


#include <glib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "timer.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/event/event.h"

#include "api.h"
#include "opengl.h"
#include "material.h"


MODULE_NAME("opengl");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The opengl module supports hardware accelerated graphics rendering and interaction by means of the freeglut library");
MODULE_VERSION(0, 5, 10);
MODULE_BCVERSION(0, 3, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 1), MODULE_DEPENDENCY("linalg", 0, 1, 4));

#ifndef GLUT_MAIN_TIMEOUT
#define GLUT_MAIN_TIMEOUT 5000
#endif

#ifndef GLUT_CLEANUP_ITERATIONS
#define GLUT_CLEANUP_ITERATIONS 2
#endif

TIMER_CALLBACK(GLUT_MAIN_LOOP);

static void openGL_idle();
static void openGL_keyDown(unsigned char key, int x, int y);
static void openGL_keyUp(unsigned char key, int x, int y);
static void openGL_specialKeyDown(int key, int x, int y);
static void openGL_specialKeyUp(int key, int x, int y);
static void openGL_reshape(int x, int y);
static void openGL_display();
static void openGL_mouse(int button, int state, int x, int y);
static void openGL_motion(int x, int y);
static void openGL_passiveMotion(int x, int y);
static void openGL_close();

static float getFloatTime();
static void freeOpenGLWindowEntry(void *window_p);

/**
 * A table of OpenGL windows registered
 */
static GHashTable *windows;
static float loopTime;

MODULE_INIT
{
	char **argv = $$(char **, getArgv)();
	int argc = $$(int, getArgc)();

	glutInit(&argc, argv);

	$$(void, setArgv)(argv);
	$$(void, setArgc)(argc);

	glutIdleFunc(&openGL_idle);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	loopTime = getFloatTime();
	windows = g_hash_table_new_full(&g_int_hash, &g_int_equal, NULL, &freeOpenGLWindowEntry);

	TIMER_ADD_TIMEOUT(GLUT_MAIN_TIMEOUT, GLUT_MAIN_LOOP);

	initOpenGLMaterials();

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_remove_all(windows);

	for(int i = 0; i < GLUT_CLEANUP_ITERATIONS; i++) { // Let freeglut perform cleanup (close all remaining windows, etc.)
		glutMainLoopEvent();
	}

	g_hash_table_destroy(windows);

	freeOpenGLMaterials();
}

TIMER_CALLBACK(GLUT_MAIN_LOOP)
{
	glutMainLoopEvent();
	TIMER_ADD_TIMEOUT(GLUT_MAIN_TIMEOUT, GLUT_MAIN_LOOP);
}

/**
 * Checks whether an OpenGL error has occurred
 *
 * @result		true if an error occurred
 */
API bool checkOpenGLError()
{
	GLenum err = glGetError();

	if(err != GL_NO_ERROR) {
		const GLubyte *errstr = gluErrorString(err);
		if(errstr != NULL) {
			LOG_ERROR("OpenGL error #%d: %s", err, errstr);
		}

		return true;
	}

	return false;
}

/**
 * Creates a new OpenGL window
 *
 * @param name			the name of the window to create
 * @result				the created window
 */
API OpenGLWindow *createOpenGLWindow(char *name)
{
	OpenGLWindow *window = ALLOCATE_OBJECT(OpenGLWindow);
	window->id = glutCreateWindow(name);
	window->active = true;

	glutSetWindow(window->id);

	glutIgnoreKeyRepeat(true);
	glutKeyboardFunc(&openGL_keyDown);
	glutKeyboardUpFunc(&openGL_keyUp);
	glutSpecialFunc(&openGL_specialKeyDown);
	glutSpecialUpFunc(&openGL_specialKeyUp);
	glutReshapeFunc(&openGL_reshape);
	glutDisplayFunc(&openGL_display);
	glutMouseFunc(&openGL_mouse);
	glutMotionFunc(&openGL_motion);
	glutPassiveMotionFunc(&openGL_passiveMotion);
	glutCloseFunc(&openGL_close);

	g_hash_table_insert(windows, &window->id, window);

	LOG_INFO("Created new OpenGL window %d with name '%s', OpenGL vendor: %s %s", window->id, name, glGetString(GL_VENDOR), glGetString(GL_VERSION));

	// Initialize GLEW as well
	GLenum err;
	if((err = glewInit()) != GLEW_OK) {
		LOG_ERROR("GLEW error #%d: %s", err, glewGetErrorString(err));
		return false;
	}

	LOG_INFO("Successfully initialized GLEW %s", glewGetString(GLEW_VERSION));

	return window;
}

/**
 * Frees an OpenGL window
 *
 * @param window		the window to free
 */
API void freeOpenGLWindow(OpenGLWindow *window)
{
	g_hash_table_remove(windows, &window->id);
}

/**
 * Returns the currently active OpenGL window
 *
 * @result		the OpenGL window that is currently active
 */
API OpenGLWindow *getCurrentOpenGLWindow()
{
	int id = glutGetWindow();

	return g_hash_table_lookup(windows, &id);
}

/**
 * Idle function called by freeglut when no other events are processed
 */
static void openGL_idle()
{
	float now = getFloatTime();
	float dt = now - loopTime;
	loopTime = now;

	GHashTableIter iter;
	int id;
	OpenGLWindow *window;
	g_hash_table_iter_init(&iter, windows);
	while(g_hash_table_iter_next(&iter, (void *) &id, (void *) &window)) {
		if(window->active) {
			glutSetWindow(window->id);
			$(int, event, triggerEvent)(window, "update", dt);
		}
	}
}

/**
 * Callback function called by freeglut when a key is pressed
 */
static void openGL_keyDown(unsigned char key, int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "keyDown", (int) key, x, y);
	}
}

/**
 * Callback function called by freeglut when a key is released
 */
static void openGL_keyUp(unsigned char key, int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "keyUp", (int) key, x, y);
	}
}

/**
 * Callback function called by freeglut when a special key is pressed
 */
static void openGL_specialKeyDown(int key, int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "specialKeyDown", key, x, y);
	}
}

/**
 * Callback function called by freeglut when a special key is released
 */
static void openGL_specialKeyUp(int key, int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "specialKeyUp", key, x, y);
	}
}

/**
 * Callback function called by freeglut when an OpenGL window is reshaped
 */
static void openGL_reshape(int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "reshape", x, y);
	}
}

/**
 * Callback function called by freeglut when an OpenGL window is to be displayed
 */
static void openGL_display()
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "display");
		glutSwapBuffers(); // double buffering
	}
}

/**
 * Callback function called by freeglut when a mouse action occurs
 */
static void openGL_mouse(int button, int state, int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		if(state == GLUT_DOWN) {
			$(int, event, triggerEvent)(window, "mouseDown", button, x, y);
		} else if (state == GLUT_UP) {
			$(int, event, triggerEvent)(window, "mouseUp", button, x, y);
		}
	}
}

/**
 * Callback function called by freeglut when a mouse motion occurs
 */
static void openGL_motion(int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "mouseMove", x, y);
	}
}

/**
 * Callback function called by freeglut when a passive mouse motion occurs
 */
static void openGL_passiveMotion(int x, int y)
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "passiveMouseMove", x, y);
	}
}

/**
 * Callback function called by freeglut when a window is closed
 */
static void openGL_close()
{
	OpenGLWindow *window = getCurrentOpenGLWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "close");
		window->active = false;
		LOG_INFO("OpenGL window %d closed", window->id);
	}
}

/**
 * Returns the current system time as float
 *
 * @result		the current system time
 */
static float getFloatTime()
{
	GTimeVal time;
	g_get_current_time(&time);
	return (G_USEC_PER_SEC * time.tv_sec + time.tv_usec) * (1 / G_USEC_PER_SEC);
}

/**
 * A GDestroyNotify function to free an OpenGLWindow from the windows hash table
 *
 * @param window_p		a pointer to the window to free
 */
static void freeOpenGLWindowEntry(void *window_p)
{
	OpenGLWindow *window = window_p;

	glutDestroyWindow(window->id);

	if(window->active) {
		window->active = false;
		LOG_INFO("OpenGL window %d closed", window->id);
	}

	free(window);
}
