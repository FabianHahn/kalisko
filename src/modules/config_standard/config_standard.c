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
#include "../../util.h"
#include "modules/config/config.h"
#include "modules/config/path.h"
#include "modules/config/write.h"
#include "modules/config/parse.h"
#include "log.h"
#include "types.h"

#include "api.h"
#include "config_standard.h"
#include "modules/config_standard/util.h"

#define CONFIG_DIR_NAME "kalisko"
#define USER_CONFIG_FILE_NAME "user.cfg"
#define USER_OVERRIDE_CONFIG_FILE_NAME "override.cfg"
#define GLOBAL_CONFIG_FILE_NAME "kalisko.cfg"
#define USER_CONFIG_DIR_PERMISSION 0700

static char *userConfigFilePath;
static char *userOverrideConfigFilePath;
static char *globalConfigFilePath;

static Config *userConfig = NULL;
static Config *userOverrideConfig = NULL;
static Config *globalConfig = NULL;

static Config *getUserConfig();
static Config *getUserOverrideConfig();
static void finalize();

MODULE_NAME("config_standard");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The config_standard module provides access to standard config files that override each other");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 2, 0));

MODULE_INIT
{
	userConfigFilePath = g_build_path("/", g_get_user_config_dir(), CONFIG_DIR_NAME, USER_CONFIG_FILE_NAME, NULL);
	userOverrideConfigFilePath = g_build_path("/", g_get_user_config_dir(), CONFIG_DIR_NAME, USER_OVERRIDE_CONFIG_FILE_NAME, NULL);

	char *globalDir = getGlobalKaliskoConfigPath();
	globalConfigFilePath = g_build_path("/", globalDir, GLOBAL_CONFIG_FILE_NAME, NULL);
	free(globalDir);

	if(!HOOK_ADD(stdConfigChanged)) {
		finalize();
		return false;
	} else {
		return true;
	}
}

MODULE_FINALIZE
{
	if(userOverrideConfig) {
		saveStandardConfig(CONFIG_USER_OVERRIDE);
	}

	finalize();
}

/**
 * Returns the configuration for the given standard configuration file.
 *
 * @param file	The standard configuration file to open
 * @return		The Config reference for the given file
 * @see	StandardConfigFiles
 */
API Config *getStandardConfig(StandardConfigFiles file)
{
	switch(file) {
		case CONFIG_USER:
			if(!userConfig) {
				userConfig = getUserConfig();
			}
			return userConfig;
		break;
		case CONFIG_USER_OVERRIDE:
			if(!userOverrideConfig) {
				userOverrideConfig = getUserOverrideConfig();
			}
			return userOverrideConfig;
		break;
		case CONFIG_GLOBAL:
			if(!globalConfig) {
				// As this module should also work for a non root account
				// we don't create the global config file if it doesn't exist.
				globalConfig = $(Config *, config, parseConfigFile)(globalConfigFilePath);
			}
			return globalConfig;
		break;
		default:
			LOG_ERROR("Unknown standard configuration file requested.");
			return NULL;
		break;
	}
}


/**
 * Saves the given standard configuration file.
 *
 * Attention: Only the "override" configuration files are writeable.
 *
 * @param file	The standard configuration file to save
 */
API void saveStandardConfig(StandardConfigFiles file)
{
	switch(file) {
		case CONFIG_USER_OVERRIDE:
			$(void, config, writeConfigFile)(userOverrideConfigFilePath, getStandardConfig(file));
		break;
		default:
			LOG_WARNING("Given standard configuration file can not be saved.");
			return;
		break;
	}
}

static Config *getUserConfig()
{
	if(!g_file_test(userConfigFilePath, G_FILE_TEST_EXISTS)) {
		char *dirPath = g_build_path("/", g_get_user_config_dir(), CONFIG_DIR_NAME, NULL);
		g_mkdir_with_parents(dirPath, USER_CONFIG_DIR_PERMISSION);

		Config *userConfig = $(Config *, config, createConfig)(USER_CONFIG_FILE_NAME);
		$(void, config, writeConfigFile)(userConfigFilePath, userConfig);

		LOG_INFO("Created new configuration file: %s", userConfigFilePath);

		free(dirPath);
		return userConfig;
	} else {
		return $(Config *, config, parseConfigFile)(userConfigFilePath);
	}
}

static Config *getUserOverrideConfig()
{
	if(!g_file_test(userOverrideConfigFilePath, G_FILE_TEST_EXISTS)) {
		char *dirPath = g_build_path("/", g_get_user_config_dir(), CONFIG_DIR_NAME, NULL);
		g_mkdir_with_parents(dirPath, USER_CONFIG_DIR_PERMISSION);

		Config *globalConfig = $(Config *, config, createConfig)(USER_OVERRIDE_CONFIG_FILE_NAME);
		$(void, config, writeConfigFile)(userOverrideConfigFilePath, globalConfig);

		LOG_INFO("Created new configuration file: %s", userOverrideConfigFilePath);

		free(dirPath);
		return globalConfig;
	} else {
		return $(Config *, config, parseConfigFile)(userOverrideConfigFilePath);
	}
}

static void finalize()
{
	free(userConfigFilePath);
	free(userOverrideConfigFilePath);
	free(globalConfigFilePath);

	if(userConfig) {
		$(void, config, freeConfig)(userConfig);
	}

	if(userOverrideConfig) {
		$(void, config, freeConfig)(userOverrideConfig);
	}

	if(globalConfig) {
		$(void, config, freeConfig)(globalConfig);
	}

	HOOK_DEL(stdConfigChanged);
}
