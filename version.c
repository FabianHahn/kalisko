/**
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Kalisko Developers nor the names of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
Version *createVersion(int major, int minor, int patch, int revision)
{
	Version *ver = allocateObject(Version);
	ver->major = major;
	ver->minor = minor;
	ver->patch = patch;
	ver->revision = revision;

	return ver;
}

/**
 * Frees a version
 *
 * @param ver		the version to free
 */
void freeVersion(Version *ver)
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
int compareVersions(Version *a, Version *b)
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
