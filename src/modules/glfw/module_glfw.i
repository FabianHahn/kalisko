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


#ifndef GLFW_MODULE_GLFW_H
#define GLFW_MODULE_GLFW_H

typedef bool GlfwHandle;


/**
 * Opens a glfw window
 *
 * @param title			the title to use for the window
 * @param width			the desired screen width
 * @param height		the desired screen height
 * @param fullscreen	whether to go to fullscreen mode
 * @param vsynch		whether to use vertical sync mode
 * @result				true if successful
 */
API bool openGlfwWindow(const char *title, int width, int height, bool fullscreen, bool vsync);

/**
 * Closes the glfw window if one was open
 *
 * @result				true if successful
 */
API bool closeGlfwWindow();

/**
 * Returns the current frame rate achieved by the glfw main loop
 *
 * @result			the frame rate in frames per second
 */
API double getGlfwFps();

/**
 * Returns the glfw handle to which event listeners can be attached
 *
 * @result				a pointer to the glfw handle
 */
API GlfwHandle *getGlfwHandle();

#endif
