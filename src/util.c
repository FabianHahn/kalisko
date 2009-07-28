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

#include <glib.h>
#include <assert.h>

#include "api.h"
#include "log.h"
#include "util.h"

#define BUF 1024

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
		logError("Failed to determine executable path");
		return NULL;
	}
#else
	if((length = readlink("/proc/self/exe", execpath, BUF - 1)) < 0) {
		logSystemError("Failed to determine executable path");
		return NULL;
	} else if(length >= BUF) {
		logWarning("Path buffer too small, truncating...");
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

	g_strfreev(parts);

	return path;
}

/**
 * A GDestroyNotify wrapper around g_hash_table_destroy
 *
 * @param table		the hash table to destroy
 */
API void destroyGHashTable(void *table)
{
	g_hash_table_destroy(table);
}
