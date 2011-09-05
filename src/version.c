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
#include <memory.h>
#define API
#include "version.h"
#include "memory_alloc.h"

/**
 * Creates a version
 *
 * @param major		the version's major number
 * @param minor		the version's minor number
 * @param patch		the version's patch number
 * @param revision	the version's revision number
 * @result			the created version
 */
API Version *createVersion(int major, int minor, int patch, int revision)
{
	Version *ver = allocateMemory(sizeof(Version));
	ver->major = major;
	ver->minor = minor;
	ver->patch = patch;
	ver->revision = revision;

	return ver;
}

/**
 * Creates a copy of a version
 *
 * @param from		the version to create a copy from
 * @result			the created copy version
 */
API Version *copyVersion(Version *from)
{
	Version *version = allocateMemory(sizeof(Version));
	return memcpy(version, from, sizeof(Version));
}

/**
 * Frees a version
 *
 * @param ver		the version to free
 */
API void freeVersion(Version *ver)
{
	free(ver);
}

/**
 * Compares two versions
 *
 * @param a			the first version to compare
 * @param b			the second version to compare
 * @result			positive if a > b, zero if a == b, negative if a < b
 */
API int compareVersions(Version *a, Version *b)
{
	if(a->major == b->major) {
		if(a->minor == b->minor) {
			if(a->patch == b->patch) {
				if(a->revision == b->revision) {
					return 0;
				} else {
					return a->revision - b->revision;
				}
			} else {
				return a->patch - b->patch;
			}
		} else {
			return a->minor - b->minor;
		}
	} else {
		return a->major - b->major;
	}
}

/**
 * Returns the string representation of a version
 *
 * @param version	the version to dump
 * @result			the string representation of the version
 */
API GString *dumpVersion(Version *version)
{
	GString *string = g_string_new("");
	g_string_append_printf(string, "%d.%d.%d-%d", version->major, version->minor, version->patch, version->revision);
	return string;
}
