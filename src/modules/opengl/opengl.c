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


MODULE_NAME("opengl");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The opengl module supports hardware accelerated graphics rendering and interaction by means of the freeglut library");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 1));

#ifndef GLUT_MAIN_TIMEOUT
#define GLUT_MAIN_TIMEOUT 5000
#endif

TIMER_CALLBACK(GLUT_MAIN_LOOP);
static void globalIdleFunc();
static float getFloatTime();

/**
 * A list of OpenGL windows registered
 */
static GQueue *windows;
static float loopTime;

MODULE_INIT
{
	char **argv = $$(char **, getArgv)();
	int argc = $$(int, getArgc)();

	glutInit(&argc, argv);

	$$(void, setArgv)(argv);
	$$(void, setArgc)(argc);

	glutIdleFunc(&globalIdleFunc);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	loopTime = getFloatTime();
	windows = g_queue_new();

	TIMER_ADD_TIMEOUT(GLUT_MAIN_TIMEOUT, GLUT_MAIN_LOOP);

	return true;
}

MODULE_FINALIZE
{
	for(GList *iter = windows->head; iter != NULL; iter = iter->next) {
		freeOpenGLWindow(iter->data);
	}

	g_queue_free(windows);
}

TIMER_CALLBACK(GLUT_MAIN_LOOP)
{
	glutMainLoopEvent();
	TIMER_ADD_TIMEOUT(GLUT_MAIN_TIMEOUT, GLUT_MAIN_LOOP);
}

/**
 * Creates a new OpenGL window
 *
 * @param name			the name of the window to create
 * @result				the created window
 */
API OpenGLWindow *createOpenGLWindow(char *name)
{
	int *window = ALLOCATE_OBJECT(int);
	*window = glutCreateWindow(name);

	g_queue_push_tail(windows, window);

	LOG_INFO("Created new OpenGL window with name '%s', OpenGL vendor: %s %s", name, glGetString(GL_VENDOR), glGetString(GL_VERSION));

	return window;
}

/**
 * Frees an OpenGL window
 *
 * @param window		the window to free
 */
API void freeOpenGLWindow(OpenGLWindow *window)
{
	glutSetWindow(*window);
	glutDestroyWindow(*window);
	free(window);
}

/**
 * Idle function called by freeglut when no other events are processed
 */
static void globalIdleFunc()
{
	float now = getFloatTime();
	float dt = now - loopTime;
	loopTime = now;

	for(GList *iter = windows->head; iter != NULL; iter = iter->next) {
		int *window = iter->data;
		glutSetWindow(*window);
		$(int, event, triggerEvent)(window, "update", dt);
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
