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
#include <stdlib.h>
#include "dll.h"
#include "hooks.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "modules/store/parse.h"
#include "modules/store/merge.h"
#include "modules/getopts/getopts.h"
#include "log.h"
#include "types.h"
#include "module.h"

#include "api.h"
#include "config.h"
#include "modules/config/util.h"

#define USER_CONFIG_FILE_NAME "user.cfg"
#define USER_OVERRIDE_CONFIG_FILE_NAME "override.cfg"
#define GLOBAL_CONFIG_FILE_NAME "kalisko.cfg"

static char *writableConfigFilePath;
static char *profilePath;

static Store *config;
static Store *writableConfig;

static void loadDefaultConfigs();
static void finalize();

MODULE_NAME("config");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The config module provides access to config files and a profile feature");
MODULE_VERSION(0, 3, 1);
MODULE_BCVERSION(0, 3, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("getopts", 0, 1, 0));

MODULE_INIT
{
	config = NULL;
	profilePath = NULL;

	loadDefaultConfigs();

	if(config == NULL) {
		LOG_ERROR("No configuration files found to work with.");
		finalize();
		return false;
	}

	// Check for --profile
	if((profilePath = $(char *, getopts, getOpt)("profile")) == NULL || *profilePath == '\0') {
		// Not found, check for -p
		if((profilePath = $(char *, getopts, getOpt)("p")) == NULL || *profilePath == '\0') {
			LOG_ERROR("Could not find profile command line option, use '-p [profile]' or '--profile=[profile]'");
			profilePath = NULL;
		}
	}

	if(profilePath) {
		LOG_DEBUG("Using profile '%s'", profilePath);
		config = $(Store *, store, getStorePath)(config, profilePath);
	}

	return true;
}

MODULE_FINALIZE
{
	finalize();
}

/**
 * Returns the configuration Store.
 *
 * This Store is the merge result of the three configuration files
 * (if they all exist).
 *
 * Although this Store can be changed or extended, it's not possible to
 * save it persistent. Because of that we recommend to <b>use it read-only</b>.
 *
 * If a profile is given, the root of the Store is the profile path.
 * <b>Example:</b>
 * One of the configuration files has a value at the Store path
 * <i>/config/user/name</i> and a value at <i>/kalisko/user/name<i>. Now, if
 * no profile is given, both paths exist in the Store returned by this function.
 * If a profile is given, say <i>config</i>, you can only access the
 * first value (<i>/config/user/name<i>) by using the path <i>/user/name</i>.
 * The root of the path is set to <i>/config<i> because of the given profile.
 *
 * @return The Store of three merged configuration files
 */
API Store *getConfig()
{
	return config;
}

/**
 * @return The given path from the configuration Store.
 */
API Store *getConfigPath(char *path)
{
	return $(Store *, store, getStorePath)(config, path);
}

/**
 * Returns the writable Store. This correspond to the user specific writable
 * configuration file. This Store can be saved (@see saveWritableConfig).
 *
 * The returned Store doesn't depend on the profile. If you have to change a
 * specific value for the current profile use @see getProfilePath and add it
 * to your path as a prefix. This is done because there are use cases where
 * modules have to change values for other profiles.
 *
 * <b>Attention:</b>
 * If you changed the Store returned by this function, don't forget to call
 * @see saveWritableConfig. Even if the function is called on finalizing this
 * module.
 *
 * <b>Note:</b>
 * There is no writable global Store. This is not possible as Kalisko is not
 * essentially run under root rights.
 *
 * @return The user specific writable Store.
 */
API Store *getWritableConfig()
{
	return writableConfig;
}

/**
 * Saves the writable store to the corresponding file.
 */
API void saveWritableConfig()
{
	$(void, store, writeStoreFile)(writableConfigFilePath, writableConfig);
}

/**
 * Returns the profile path. This can be used to change values for the current
 * profile. See @see getWritableConfig.
 *
 * <b>Attention:</b>
 * The returned value doesn't have to be free'd. It's also not wise to change the
 * Value even if it's possible.
 *
 * @return The profile path of the current profile.
 */
API char *getProfilePath()
{
	return profilePath;
}

static void loadDefaultConfigs()
{
	// prepare paths
	char *userDir = getUserKaliskoConfigPath();
	char *userConfigFilePath = g_build_path("/", userDir, USER_CONFIG_FILE_NAME, NULL);
	writableConfigFilePath = g_build_path("/", userDir, USER_OVERRIDE_CONFIG_FILE_NAME, NULL);
	char *globalConfigFilePath = g_build_path("/", getGlobalKaliskoConfigPath(), GLOBAL_CONFIG_FILE_NAME, NULL);

	if(g_file_test(globalConfigFilePath, G_FILE_TEST_EXISTS)) {
		config = $(Store *, store, parseStoreFile)(globalConfigFilePath);
	}

	if(g_file_test(userConfigFilePath, G_FILE_TEST_EXISTS)) {
		Store *userConfig = $(Store *, store, parseStoreFile)(userConfigFilePath);

		if(config == NULL) {
			config = userConfig;
		} else {
			if(!$(bool, store, mergeStore)(config, userConfig)) {
				LOG_WARNING("Could not merge global config store with user config store.");
			}

			$(void, store, freeStore)(userConfig);
		}
	}

	if(g_file_test(writableConfigFilePath, G_FILE_TEST_EXISTS)) {
		Store *writableConfig = $(Store *, store, parseStoreFile)(writableConfigFilePath);

		if(config == NULL) {
			config = writableConfig;
		} else {
			if(!$(bool, store, mergeStore)(config, writableConfig)) {
				LOG_WARNING("Could not merge writable store into existing config store.");
			}

			$(void, store, freeStore)(writableConfig);
		}
	}

	// free locally used paths
	free(globalConfigFilePath);
	free(userConfigFilePath);
	free(userDir);
}

static void finalize()
{
	if(writableConfig) {
		saveWritableConfig();
		$(void, store, freeStore)(writableConfig);
	}

	if(writableConfigFilePath) {
		free(writableConfigFilePath);
	}

	if(config) {
		$(void, store, freeStore)(config);
	}

}
