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

#define API
#include "log.h"
#include "util.h"

#define BUF 1024

static char **argv;
static int argc;

API char **getArgv()
{
	return argv;
}

API void setArgv(char **args)
{
	argv = args;
}

API int getArgc()
{
	return argc;
}

API void setArgc(int count)
{
	argc = count;
}

API char *getExecutablePath()
{
	int length;
	char execpath[BUF];

#ifdef WIN32
	if((length = GetModuleFileName(NULL, execpath, BUF - 1)) == 0) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to determine executable path");
		return NULL;
	}
#else
	if((length = readlink("/proc/self/exe", execpath, BUF - 1)) < 0) {
		logMessage("core", LOG_LEVEL_ERROR, "Failed to determine executable path: %s", strerror(errno));
		return NULL;
	} else if(length >= BUF) {
		logMessage("core", LOG_LEVEL_WARNING, "Path buffer too small, truncating...");
	}
#endif
	execpath[length] = '\0'; // Terminate string

	return getDirectoryPath(execpath);
}

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

API int compareIntegers(const void *a, const void *b, void *data)
{
	int va = *((int *) a);
	int vb = *((int *) b);
	return va - vb;
}

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

API void breakpoint()
{
	int i = 0;
	i++;
}

API double getMicroTime()
{
	GTimeVal time;
	g_get_current_time(&time);

	return time.tv_sec + (double) time.tv_usec / G_USEC_PER_SEC;
}

API int getTime()
{
	GTimeVal time;
	g_get_current_time(&time);
	return time.tv_sec;
}

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
