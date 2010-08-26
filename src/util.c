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


#ifdef WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#include <windows.h>
#else
#include <unistd.h> // readlink
#endif

#include <stdio.h>
#include <glib.h>
#include <assert.h>

#include "api.h"
#include "log.h"
#include "util.h"

#define BUF 1024

static char **argv;
static int argc;

/**
 * Returns the argv from main().
 *
 * @return The argv from main().
 */
API char **getArgv()
{
	return argv;
}

/**
 * Sets the argv.
 *
 * @param args		The argv from main().
 */
API void setArgv(char **args)
{
	argv = args;
}

/**
 * Returns the argc from main().
 *
 * @return The argc from main().
 */
API int getArgc()
{
	return argc;
}

/**
 * Sets the argc.
 *
 * @param count	The argc from main().
 */
API void setArgc(int count)
{
	argc = count;
}

/**
 * Returns the path to the core executable
 *
 * @result		the executable's path
 */
API char *getExecutablePath()
{
	int length;
	char execpath[BUF];

#ifdef WIN32
	if((length = GetModuleFileName(NULL, execpath, BUF - 1)) == 0) {
		logMessage(LOG_TYPE_ERROR, "Failed to determine executable path");
		return NULL;
	}
#else
	if((length = readlink("/proc/self/exe", execpath, BUF - 1)) < 0) {
		logMessage(LOG_TYPE_ERROR, "Failed to determine executable path: %s", strerror(errno));
		return NULL;
	} else if(length >= BUF) {
		logMessage(LOG_TYPE_WARNING, "Path buffer too small, truncating...");
	}
#endif
	execpath[length] = '\0'; // Terminate string

	return getDirectoryPath(execpath);
}

/**
 * Returns the path for the parents directory of a file path (removes the file from the path).
 *
 * @param filePath	The path to a file
 * @return The path to the parents directory. Must be freed
 */
API char *getDirectoryPath(char *filePath)
{
	int i;
	char *temp;
	char **parts = g_strsplit_set(filePath, "/\\", 0); // Split the path by path delimiters

	for(i = 0; parts[i] != NULL; i++); // Find the NULL terminator
	assert(i > 0);

	temp = parts[i-1]; // Remove last element
	parts[i-1] = NULL;

	char *path = g_strjoinv("/", parts); // Construct executable path

    parts[i-1] = temp; // Restore last element so it gets freed as well

	g_strfreev(parts);

	return path;
}

/**
 * A GCompareDataFunc for integers
 *
 * @param a		the first number to compare
 * @param b		the second number to compare
 * @param data	unused
 * @result		negative if a < b, zero if a = b, positive if a > b
 */
API int compareIntegers(const void *a, const void *b, void *data)
{
	int va = *((int *) a);
	int vb = *((int *) b);
	return va - vb;
}

/**
 * A GCompareDataFunc for GTimeVals
 *
 * @param a		the first time to compare
 * @param b		the second time to compare
 * @param data	unused
 * @result		negative if a < b, zero if a = b, positive if a > b
 */
API int compareTimes(const void *a, const void *b, void *data)
{
	GTimeVal *timea = (GTimeVal *) a;
	GTimeVal *timeb = (GTimeVal *) b;
	glong diff;

	if((diff = timea->tv_sec - timeb->tv_sec) > 0) {
		return 1;
	} else if(diff < 0) {
		return -1;
	} else {
		if((diff = timea->tv_usec - timeb->tv_usec) > 0) {
			return 1;
		} else if(diff < 0) {
			return -1;
		} else {
			return 0;
		}
	}
}

/**
 * Utility function to set breakpoints if the debugger doesn't support setting module breakpoints from the beginning of execution
 * Usage: Just call this function whereever you'd like to set a breakpoint inside a module but set the breakpoint in your debugger HERE
 */
API void breakpoint()
{
	int i = 0;
	i++;
}

/**
 * Returns the current time in seconds with microsecond precision
 *
 * @result		the current time in seconds with microsecond precision
 */
API double getMicroTime()
{
	GTimeVal time;
	g_get_current_time(&time);

	return time.tv_sec + (double) time.tv_usec / G_USEC_PER_SEC;
}

/**
 * Returns the current time in seconds
 *
 * @result		the current time in seconds
 */
API int getTime()
{
	GTimeVal time;
	g_get_current_time(&time);
	return time.tv_sec;
}

/**
 * Returns the executable name.
 *
 * @result		the name of the executable. Must be freed after use.
 */
API char *getExecutableName()
{
	char *name = g_get_prgname();
	if(name) {
		return strdup(name);
	} else {
		char **argv = getArgv();
		char *firstArg = argv[0];

		if(!firstArg) {
			return strdup("[unknown]");
		}

#ifdef WIN32
		char *nameStart = strrchr(firstArg, '\\');
#else
		char *nameStart = strrchr(firstArg, '/');
#endif
		if(!nameStart) {
			return strdup("[unknown]");
		}

		return strdup(++nameStart);
	}
}
