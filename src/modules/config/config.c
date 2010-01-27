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
#include "log.h"
#include "types.h"
#include "module.h"

#include "api.h"
#include "config.h"
#include "modules/config/util.h"

#define CONFIG_DIR_NAME "kalisko"
#define USER_CONFIG_FILE_NAME "user.cfg"
#define USER_OVERRIDE_CONFIG_FILE_NAME "override.cfg"
#define GLOBAL_CONFIG_FILE_NAME "kalisko.cfg"
#define USER_CONFIG_DIR_PERMISSION 0700

static char *userConfigFilePath;
static char *userOverrideConfigFilePath;
static char *globalConfigFilePath;

static Store *userConfig = NULL;
static Store *userOverrideConfig = NULL;
static Store *globalConfig = NULL;

static Store *getUserConfig();
static Store *getUserOverrideConfig();
static void finalize();

MODULE_NAME("config");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The config module provides access to config files represented by a store that override each other");
MODULE_VERSION(0, 2, 2);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 5, 3));

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
		saveConfig(CONFIG_USER_OVERRIDE);
	}

	finalize();
}

/**
 * Returns the configuration for the given configuration file.
 *
 * @param file	The configuration file to open
 * @return		The Config reference for the given file
 * @see	ConfigFiles
 */
API Store *getConfig(ConfigFiles file)
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
				globalConfig = $(Store *, store, parseStoreFile)(globalConfigFilePath);
			}
			return globalConfig;
		break;
		default:
			LOG_ERROR("Unknown configuration file requested.");
			return NULL;
		break;
	}
}


/**
 * Saves the given configuration file.
 *
 * Attention: Only the "override" configuration files are writeable.
 *
 * @param file	The configuration file to save
 */
API void saveConfig(ConfigFiles file)
{
	switch(file) {
		case CONFIG_USER_OVERRIDE:
			$(void, store, writeStoreFile)(userOverrideConfigFilePath, getConfig(file));
		break;
		default:
			LOG_WARNING("Given configuration file can not be saved.");
			return;
		break;
	}
}

/**
 * Searches for the given path trough the configuration files
 * consider the weighting of the different configurations. The first found value
 * will be returned otherwise NULL.
 *
 * Do not free the returned value. This is handled by the config module.
 *
 * @param	path			The path to search
 * @return	The first found value for given path or NULL
 */
API Store *getConfigPathValue(char *path)
{
	Store *value = NULL;

	Store *overrideConfig = getConfig(CONFIG_USER_OVERRIDE);
	if(overrideConfig) {
		value = $(Store *, store, getStorePath)(overrideConfig, path);
		if(value) {
			return value;
		}
	}

	Store *userConfig = getConfig(CONFIG_USER);
	if(userConfig) {
		value = $(Store *, store, getStorePath)(userConfig, path);
		if(value) {
			return value;
		}
	}

	Store *globalConfig = getConfig(CONFIG_GLOBAL);
	if(globalConfig) {
		value = $(Store *, store, getStorePath)(globalConfig, path);
		if(value) {
			return value;
		}
	}

	return NULL;
}

static Store *getUserConfig()
{
	if(!g_file_test(userConfigFilePath, G_FILE_TEST_EXISTS)) {
		char *dirPath = g_build_path("/", g_get_user_config_dir(), CONFIG_DIR_NAME, NULL);
		g_mkdir_with_parents(dirPath, USER_CONFIG_DIR_PERMISSION);

		Store *userConfig = $(Store *, store, createStore)();
		$(void, store, writeStoreFile)(userConfigFilePath, userConfig);

		LOG_INFO("Created new configuration file: %s", userConfigFilePath);

		free(dirPath);
		return userConfig;
	} else {
		return $(Store *, store, parseStoreFile)(userConfigFilePath);
	}
}

static Store *getUserOverrideConfig()
{
	if(!g_file_test(userOverrideConfigFilePath, G_FILE_TEST_EXISTS)) {
		char *dirPath = g_build_path("/", g_get_user_config_dir(), CONFIG_DIR_NAME, NULL);
		g_mkdir_with_parents(dirPath, USER_CONFIG_DIR_PERMISSION);
		
		Store *globalConfig = $(Store *, store, createStore)();
		$(void, store, writeStoreFile)(userOverrideConfigFilePath, globalConfig);
		
		LOG_INFO("Created new configuration file: %s", userOverrideConfigFilePath);

		free(dirPath);
		return globalConfig;
	} else {
		return $(Store *, store, parseStoreFile)(userOverrideConfigFilePath);
	}
}

static void finalize()
{
	free(userConfigFilePath);
	free(userOverrideConfigFilePath);
	free(globalConfigFilePath);

	if(userConfig) {
		$(void, store, freeStore)(userConfig);
	}

	if(userOverrideConfig) {
		$(void, store, freeStore)(userOverrideConfig);
	}

	if(globalConfig) {
		$(void, store, freeStore)(globalConfig);
	}

	HOOK_DEL(stdConfigChanged);
}
