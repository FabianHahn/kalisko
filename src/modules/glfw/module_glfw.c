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
#include <GL/glfw.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "timer.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/event/event.h"
#include "api.h"
#include "module_glfw.h"

MODULE_NAME("glfw");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to use glfw as an OpenGL context provider for high performance applications");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("event", 0, 2, 1));

TIMER_CALLBACK(GLFW_MAIN_LOOP);
static double getDoubleTime();
static bool windowOpen;
static double loopTime;

MODULE_INIT
{
	if(!glfwInit()) {
		return false;
	}

	TIMER_ADD_TIMEOUT(0, GLFW_MAIN_LOOP);

	return true;
}

MODULE_FINALIZE
{
	glfwTerminate();
}

TIMER_CALLBACK(GLFW_MAIN_LOOP)
{
	double now = getDoubleTime();
	double dt = now - loopTime;
	loopTime = now;

	$(int, event, triggerEvent)(&windowOpen, "update", dt);
	$(int, event, triggerEvent)(&windowOpen, "display");
	glfwSwapBuffers();

	if(glfwGetWindowParam(GLFW_OPENED)) { // continue as long as window is opened
		TIMER_ADD_TIMEOUT(0, GLFW_MAIN_LOOP);
	} else {
		$(int, event, triggerEvent)(&windowOpen, "close");
		windowOpen = false;
	}
}

/**
 * Opens a glfw window
 *
 * @param title			the title to use for the window
 * @param width			the desired screen width
 * @param height		the desired screen height
 * @param fullscreen	whether to go to fullscreen mode
 * @result				true if successful
 */
API bool openGlfwWindow(const char *title, int width, int height, bool fullscreen)
{
	if(windowOpen) {
		LOG_ERROR("Failed to open glfw window: Only one glfw window can be opened at the same time");
		return false;
	}

	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);

	int mode = fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW;

	if(glfwOpenWindow(width, height, 8, 8, 8, 8, 8, 0, mode) == GL_FALSE) {
		LOG_ERROR("Failed to open glfw window");
		return false;
	}

	glfwSetWindowTitle(title);

	windowOpen = true;

	return true;
}

/**
 * Closes the glfw window if one was open
 *
 * @result				true if successful
 */
API bool closeGlfwWindow()
{
	if(!windowOpen) {
		return false;
	}

	glfwCloseWindow();
	windowOpen = false;

	$(int, event, triggerEvent)(&windowOpen, "close");

	return true;
}

/**
 * Returns the current frame rate achieved by the glfw main loop
 *
 * @result			the frame rate in frames per second
 */
API double getGlfwFps()
{
	return 1.0 / loopTime;
}

/**
 * Returns the glfw handle to which event listeners can be attached
 *
 * @result				a pointer to the glfw handle
 */
API GlfwHandle *getGlfwHandle()
{
	return &windowOpen;
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
