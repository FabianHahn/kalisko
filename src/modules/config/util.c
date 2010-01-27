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
#include "dll.h"
#include "types.h"
#include "log.h"
#include "modules/store/store.h"
#include "modules/store/path.h"

#include "api.h"
#include "config.h"
#include "util.h"

#define KALISKO_DIR_NAME "kalisko"

/**
 * Returns the path to the Kalisko specific system wide configuration directory. This directory must not exist
 * yet.
 *
 * @return	The Kalisko specific folder path for system wide
 * 			configurations. This string must be freed.
 */
API char *getGlobalKaliskoConfigPath()
{
// On Unix/Linux systems GLib returns a path as defined in "XDG Base Direcotry Specification".
// This path is not the right one for Kalisko as it is more for X Window applications and Kalisko is
// not necessarily a X Window application.
#if defined(__unix__) || defined(__linux__)
	#ifdef __FreeBsd__
		return g_build_path("/", "/usr/local/etc", KALISKO_DIR_NAME, NULL);
	#else
		return g_build_path("/", "/etc", KALISKO_DIR_NAME, NULL);
	#endif
#else
// However, on Windows systems GLib returns the right path as it uses a Windows API function. For this
// case we use the GLib function (and also for other systems, which are not Unix/Linux based).
	char const * const *globalConfigDirectories = g_get_system_config_dirs();
	if(globalConfigDirectories[0] == NULL) {
		LOG_INFO("Could not find a system wide configuration directory. Using the executable directory.");
		return g_build_path("/", $$(char *, getExecutablePath)(), KALISKO_DIR_NAME, NULL);
	} else {
		return g_build_path("/", globalConfigDirectories[0], KALISKO_DIR_NAME, NULL);
	}
#endif
}
