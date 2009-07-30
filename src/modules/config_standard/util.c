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
#include "modules/config/config.h"
#include "modules/config/path.h"

#include "api.h"
#include "config_standard.h"
#include "util.h"

/**
 * Searches for the given path trough the standard configuration files
 * consider the weighting of the different configurations. The first found value
 * will be returned otherwise NULL.
 *
 * Do not free the returned value. This is handled by the config_standard module.
 *
 * @param	path			The path to search
 * @return	The first found value for given path or NULL
 */
API ConfigNodeValue *getStandardConfigPathValue(char *path)
{
	ConfigNodeValue *value = NULL;

	Config *overrideConfig = getStandardConfig(CONFIG_USER_OVERRIDE);
	if(overrideConfig) {
		value = $(ConfigNodeValue *, config, getConfigPath)(overrideConfig, path);
		if(value) {
			return value;
		}
	}

	Config *userConfig = getStandardConfig(CONFIG_USER);
	if(userConfig) {
		value = $(ConfigNodeValue *, config, getConfigPath)(userConfig, path);
		if(value) {
			return value;
		}
	}

	Config *globalConfig = getStandardConfig(CONFIG_GLOBAL);
	if(globalConfig) {
		value = $(ConfigNodeValue *, config, getConfigPath)(globalConfig, path);
		if(value) {
			return value;
		}
	}

	return NULL;
}
