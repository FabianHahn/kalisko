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

#define API
#include "freeglut.h"

MODULE_NAME("freeglut");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The freeglut module is an freeglut context provider suited to rendering into multiple windows");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 1));

#ifndef FREEGLUT_MAIN_TIMEOUT
#define FREEGLUT_MAIN_TIMEOUT 5000
#endif

#ifndef FREEGLUT_CLEANUP_ITERATIONS
#define FREEGLUT_CLEANUP_ITERATIONS 2
#endif

TIMER_CALLBACK(FREEGLUT_MAIN_LOOP);

static void freeglut_idle();
static void freeglut_keyDown(unsigned char key, int x, int y);
static void freeglut_keyUp(unsigned char key, int x, int y);
static void freeglut_specialKeyDown(int key, int x, int y);
static void freeglut_specialKeyUp(int key, int x, int y);
static void freeglut_reshape(int w, int h);
static void freeglut_display();
static void freeglut_mouse(int button, int state, int x, int y);
static void freeglut_motion(int x, int y);
static void freeglut_passiveMotion(int x, int y);
static void freeglut_close();

static double getDoubleTime();
static void freeFreeglutWindowEntry(void *window_p);

/**
 * A table of freeglut windows registered
 */
static GHashTable *windows;
static double loopTime;

MODULE_INIT
{
	char **argv = $$(char **, getArgv)();
	int argc = $$(int, getArgc)();

	glutInit(&argc, argv);

	$$(void, setArgv)(argv);
	$$(void, setArgc)(argc);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);

	loopTime = getDoubleTime();
	windows = g_hash_table_new_full(&g_int_hash, &g_int_equal, NULL, &freeFreeglutWindowEntry);

	TIMER_ADD_TIMEOUT(FREEGLUT_MAIN_TIMEOUT, FREEGLUT_MAIN_LOOP);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_remove_all(windows);

	for(int i = 0; i < FREEGLUT_CLEANUP_ITERATIONS; i++) { // Let freeglut perform cleanup (close all remaining windows, etc.)
		glutMainLoopEvent();
	}

	g_hash_table_destroy(windows);
}

TIMER_CALLBACK(FREEGLUT_MAIN_LOOP)
{
	freeglut_idle();
	glutMainLoopEvent();
	TIMER_ADD_TIMEOUT(FREEGLUT_MAIN_TIMEOUT, FREEGLUT_MAIN_LOOP);
}

API FreeglutWindow *createFreeglutWindow(char *name)
{
	FreeglutWindow *window = ALLOCATE_OBJECT(FreeglutWindow);
	window->id = glutCreateWindow(name);
	window->active = true;

	glutSetWindow(window->id);

	glutIgnoreKeyRepeat(true);
	glutKeyboardFunc(&freeglut_keyDown);
	glutKeyboardUpFunc(&freeglut_keyUp);
	glutSpecialFunc(&freeglut_specialKeyDown);
	glutSpecialUpFunc(&freeglut_specialKeyUp);
	glutReshapeFunc(&freeglut_reshape);
	glutDisplayFunc(&freeglut_display);
	glutMouseFunc(&freeglut_mouse);
	glutMotionFunc(&freeglut_motion);
	glutPassiveMotionFunc(&freeglut_passiveMotion);
	glutCloseFunc(&freeglut_close);

	g_hash_table_insert(windows, &window->id, window);

	LOG_INFO("Created new Freeglut window %d with name '%s', OpenGL vendor: %s %s", window->id, name, glGetString(GL_VENDOR), glGetString(GL_VERSION));

	// Initialize GLEW as well
	GLenum err;
	if((err = glewInit()) != GLEW_OK) {
		LOG_ERROR("GLEW error #%d: %s", err, glewGetErrorString(err));
		return NULL;
	}

	LOG_INFO("Successfully initialized GLEW %s", glewGetString(GLEW_VERSION));

	return window;
}

API void freeFreeglutWindow(FreeglutWindow *window)
{
	g_hash_table_remove(windows, &window->id);
}

API FreeglutWindow *getCurrentFreeglutWindow()
{
	int id = glutGetWindow();

	return g_hash_table_lookup(windows, &id);
}

/**
 * Idle function called by freeglut when no other events are processed
 */
static void freeglut_idle()
{
	double now = getDoubleTime();
	double dt = now - loopTime;
	loopTime = now;

	GHashTableIter iter;
	int *id;
	FreeglutWindow *window;
	g_hash_table_iter_init(&iter, windows);
	while(g_hash_table_iter_next(&iter, (void **) &id, (void **) &window)) {
		if(window->active) {
			glutSetWindow(window->id);
			$(int, event, triggerEvent)(window, "update", dt);
		}
	}
}

/**
 * Callback function called by freeglut when a key is pressed
 */
static void freeglut_keyDown(unsigned char key, int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "keyDown", (int) key, x, y);
	}
}

/**
 * Callback function called by freeglut when a key is released
 */
static void freeglut_keyUp(unsigned char key, int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "keyUp", (int) key, x, y);
	}
}

/**
 * Callback function called by freeglut when a special key is pressed
 */
static void freeglut_specialKeyDown(int key, int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "specialKeyDown", key, x, y);
	}
}

/**
 * Callback function called by freeglut when a special key is released
 */
static void freeglut_specialKeyUp(int key, int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "specialKeyUp", key, x, y);
	}
}

/**
 * Callback function called by freeglut when an freeglut window is reshaped
 */
static void freeglut_reshape(int w, int h)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "reshape", w, h);
	}
}

/**
 * Callback function called by freeglut when an freeglut window is to be displayed
 */
static void freeglut_display()
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "display");
		glutSwapBuffers(); // double buffering
	}
}

/**
 * Callback function called by freeglut when a mouse action occurs
 */
static void freeglut_mouse(int button, int state, int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

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
static void freeglut_motion(int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "mouseMove", x, y);
	}
}

/**
 * Callback function called by freeglut when a passive mouse motion occurs
 */
static void freeglut_passiveMotion(int x, int y)
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "passiveMouseMove", x, y);
	}
}

/**
 * Callback function called by freeglut when a window is closed
 */
static void freeglut_close()
{
	FreeglutWindow *window = getCurrentFreeglutWindow();

	if(window != NULL && window->active) {
		$(int, event, triggerEvent)(window, "close");
		window->active = false;
		LOG_INFO("freeglut window %d closed", window->id);
	}
}

/**
 * Returns the current system time as double
 *
 * @result		the current system time
 */
static double getDoubleTime()
{
	GTimeVal time;
	g_get_current_time(&time);
	return (G_USEC_PER_SEC * time.tv_sec + time.tv_usec) * (1.0 / G_USEC_PER_SEC);
}

/**
 * A GDestroyNotify function to free an freeglutWindow from the windows hash table
 *
 * @param window_p		a pointer to the window to free
 */
static void freeFreeglutWindowEntry(void *window_p)
{
	FreeglutWindow *window = window_p;

	glutDestroyWindow(window->id);

	if(window->active) {
		window->active = false;
		LOG_INFO("Freeglut window %d closed", window->id);
	}

	free(window);
}
