/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "modules/store/parse.h"
#include "modules/store/merge.h"
#include "modules/store/clone.h"
#include "modules/getopts/getopts.h"
#include "modules/event/event.h"
#include "log.h"
#include "types.h"
#include "module.h"

#define API
#include "config.h"
#include "modules/config/util.h"

#define USER_CONFIG_FILE_NAME "user.cfg"
#define USER_OVERRIDE_CONFIG_FILE_NAME "override.cfg"
#define PROFILES_CONFIG_FILE_NAME "profiles.cfg"
#define GLOBAL_CONFIG_FILE_NAME "global.cfg"
#define MERGE_CONFIG_PATH "merge"
#define USER_CONFIG_DIR_PERMISSION 0700

static char *writableConfigFilePath;
static char *profilePath;
static char *cliConfigFilePath;
static bool writableConfigPathExists;

// This Store contains the read-only configs and writable one merged together
static Store *config;

// The Store representing the writable config
static Store *writableConfig;

// The Store containing only the read-only configs
static Store *readOnlyConfig;

static Store *loadReadOnlyConfigs();
static Store *loadWritableConfig();
static void checkFilesMerge(Store *store);
static void finalize();
static bool internalReloadConfig(bool doTriggerEvent);

MODULE_NAME("config");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The config module provides access to config files and a profile feature");
MODULE_VERSION(0, 4, 3);
MODULE_BCVERSION(0, 3, 8);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 12), MODULE_DEPENDENCY("getopts", 0, 1, 0), MODULE_DEPENDENCY("event", 0, 1, 1));

MODULE_INIT
{
	// initialize global variables
	writableConfigFilePath = NULL;
	profilePath = NULL;
	cliConfigFilePath = NULL;
	config = NULL;
	writableConfig = NULL;
	readOnlyConfig = NULL;
	writableConfigPathExists = false;

	// get CLI options
	char *cliOptConfigFilePath = $(char *, getopts, getOptValue)("config", "c", NULL);

	if(cliOptConfigFilePath != NULL) {
		cliConfigFilePath = cliOptConfigFilePath;
	}

	char *cliOptProfilePath = $(char *, getopts, getOptValue)("profile", "p", NULL);
	if(cliOptProfilePath) {
		LOG_DEBUG("Got following profile: %s", cliOptProfilePath);
		profilePath = cliOptProfilePath;
	}

	if(!internalReloadConfig(false)) {
		finalize();
		return false;
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
 * <i>/config/user/name</i> and a value at <i>/kalisko/user/name</i>. Now, if
 * no profile is given, both paths exist in the Store returned by this function.
 * If a profile is given, say <i>config</i>, you can only access the
 * first value (<i>/config/user/name</i>) by using the path <i>/user/name</i>.
 * The root of the path is set to <i>/config</i> because of the given profile.
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
	return getStorePath(config, "%s", path);
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
 *
 * After saving the new writable config is merged together to a new global config.
 * So if you want to use the new settings in the writable config you must call this
 * function.
 *
 * At the end the event "savedWritableConfig" is triggered.
 */
API void saveWritableConfig()
{
	if(writableConfig == NULL) {
		return;
	}

	if(writableConfigPathExists) {
		// save to disk
		$(void, store, writeStoreFile)(writableConfigFilePath, writableConfig);
	} else {
		LOG_ERROR("Writable configuration file cannot be saved. Look for previous error messages");
	}

	// update config
	Store *oldConfig = config;

	config = $(Store *, store, cloneStore)(readOnlyConfig);
	if(!$(Store *, store, mergeStore)(config, writableConfig)) {
		LOG_ERROR("Could not merge read-only and writable config Stores, using old config.");
		$(void, store, freeStore)(config);
		config = oldConfig;

		return;
	}

	// event
	$(int, event, triggerEvent)(NULL, "savedWritableConfig", NULL);
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

/**
 * Reloads the configuration files (read-only and writable) and triggers at the end
 * the 'reloadedConfig' event to notify modules about the change.
 *
 * The writable one is a special case. This one is just loaded once as it is managed by
 * the application and there is no point to reload it.
 */
API void reloadConfig()
{
	internalReloadConfig(true);
}

/**
 * <b>Only for testing</b>. Injects the given store as the read-only config Store.
 *
 * The new config is merged into a new read-only & writable config.
 *
 * @param new	The Store to inject
 * @return The old Store. Must be freed
 */
API Store* injectReadOnlyConfig(Store* new)
{
	Store *old = readOnlyConfig;
	readOnlyConfig = new;

	Store *oldConfig = config;
	config = $(Store *, store, cloneStore)(readOnlyConfig);

	if(!$(Store *, store, mergeStore)(config, writableConfig)) {
		LOG_ERROR("inject: Could not merge read-only and writable config Stores.");
		$(void, store, freeStore)(config);
		config = oldConfig;
		readOnlyConfig = old;

		return NULL;
	}

	$(void, store, freeStore)(oldConfig);

	return old;
}

/**
 * <b>Only for testing</b>. Injects the given store as the writable config Store.
 *
 * The new config is merged into a new read-only & writable config.
 *
 * @param new				The Store to inject
 * @param updateConfig		Whether the config should be reloaded
 * @return The old Store. Must be freed
 */
API Store* injectWritableConfig(Store* new, bool updateConfig)
{
	Store *old = writableConfig;
	writableConfig = new;

	if(updateConfig) {
		Store *oldConfig = config;
		config = $(Store *, store, cloneStore)(readOnlyConfig);

		if(!$(Store *, store, mergeStore)(config, writableConfig)) {
			LOG_ERROR("inject: Could not merge read-only and writable config Stores.");
			$(void, store, freeStore)(config);
			config = oldConfig;
			writableConfig = old;

			return NULL;
		}

		$(void, store, freeStore)(oldConfig);
	}

	return old;
}

/**
 * <b>Only for testing</b>. Injects the given file path as the writable config file path.
 *
 * Does only change the paths. Config Stores are still the same after the call.
 *
 * @param filePath	The new file path to set. Must be allocated so the module can free it
 * @return The old file path. Must be freed
 */
API char* injectWritableConfigFilePath(char *filePath)
{
	char *old = writableConfigFilePath;
	writableConfigFilePath = filePath;

	return old;
}

/**
 * <b>Only for testing</b>. Injects the given config profile path.
 *
 * @param profilePath	The new profile path to set. Must be allocated so the module can free it
 * @return The old profile path. Must be freed
 */
API char* injectConfigProfile(char *path)
{
	char *old = profilePath;
	profilePath = path;

	return old;
}

/**
 * Reloads all configuration files and processes them. This function
 * does initialize all the global Store * variables.
 *
 * @param doTriggerEvent
 */
static bool internalReloadConfig(bool doTriggerEvent)
{
	Store *oldConfig = config;
	config = NULL;

	if(readOnlyConfig) {
		$(void, store, freeStore)(readOnlyConfig);
	}
	readOnlyConfig = NULL;

	if(writableConfig) {
		$(void, store, freeStore)(writableConfig);
	}
	writableConfig = NULL;

	// get read-only config
	readOnlyConfig = loadReadOnlyConfigs();
	if(readOnlyConfig == NULL) {
		return false;
	}

	// get the writable config
	writableConfig = loadWritableConfig(); // does never return NULL

	// merge read-only and writable configs
	config = $(Store *, store, cloneStore)(readOnlyConfig);

	if(!$(Store *, store, mergeStore)(config, writableConfig)) {
		LOG_ERROR("Could not merge read-only and writable config Stores.");

		return false;
	}

	if(doTriggerEvent) {
		$(int, event, triggerEvent)(NULL, "reloadedConfig", oldConfig);
	}

	if(oldConfig) {
		$(void, store, freeStore)(oldConfig);
	}

	return true;
}

/**
 * Loads the read-only configuration files in the right order, merges them and applies
 * the profile path.
 *
 * @return The processed read-onlystatic void internalReloadConfig(bool triggerEvent) configuration files as a Store. Returns NULL on error
 */
static Store *loadReadOnlyConfigs()
{
	Store* retReadOnlyConfig = NULL;

	if(cliConfigFilePath != NULL) {
		Store *cmdConfig = $(Store *, store, parseStoreFile)(cliConfigFilePath);

		if(cmdConfig == NULL) {
			LOG_ERROR("Given file path could not be read and used as configuration file: %s", cliConfigFilePath);
			return NULL;
		}

		checkFilesMerge(cmdConfig); // without profile

		if(profilePath != NULL) {
			Store *cmdConfigWithProfile = getStorePath(cmdConfig, "%s", profilePath);

			if(cmdConfigWithProfile == NULL) {
				LOG_ERROR("Given CLI configuration file has not the given profile path in it: %s", cliConfigFilePath);
				$(void, store, freeStore)(cmdConfig);

				return NULL;
			}
			cmdConfig = cmdConfigWithProfile;

			checkFilesMerge(cmdConfig); // with profile
		}

		retReadOnlyConfig = cmdConfig;
	} else { // we load the default configuration files
		// prepare paths
		char *userDir = getUserKaliskoConfigPath();
		char *userConfigFilePath = g_build_path("/", userDir, USER_CONFIG_FILE_NAME, NULL);
		char *profilesConfigFilePath = g_build_path("/", getGlobalKaliskoConfigPath(), PROFILES_CONFIG_FILE_NAME, NULL);
		char *globalConfigFilePath = g_build_path ("/", getGlobalKaliskoConfigPath(), GLOBAL_CONFIG_FILE_NAME, NULL);

		LOG_DEBUG("Expecting user config at: %s", userConfigFilePath);
		LOG_DEBUG("Expecting profiles config at: %s", profilesConfigFilePath);
		LOG_DEBUG("Expecting global config at: %s", globalConfigFilePath);

		// load files and merge them
		if(g_file_test(profilesConfigFilePath, G_FILE_TEST_EXISTS)) {
			retReadOnlyConfig = $(Store *, store, parseStoreFile)(profilesConfigFilePath);
			LOG_DEBUG("Loaded profiles config");
		}

		if(g_file_test(userConfigFilePath, G_FILE_TEST_EXISTS)) {
			Store *userConfig = $(Store *, store, parseStoreFile)(userConfigFilePath);

			if(retReadOnlyConfig == NULL) {
				retReadOnlyConfig = userConfig;
				LOG_DEBUG("Loaded user config");
			} else {
				if(!$(bool, store, mergeStore)(retReadOnlyConfig, userConfig)) {
					LOG_WARNING("Could not merge user config with profiles config");
				} else {
					LOG_DEBUG("Loaded user config and merged it into the profiles config");
				}

				$(void, store, freeStore)(userConfig);
			}
		}

		// clean up not needed file paths
		free(userConfigFilePath);
		free(profilesConfigFilePath);
		free(userDir);

		// Check if we found any config. If not we just create an empty one
		if(retReadOnlyConfig == NULL) {
			retReadOnlyConfig = $(Store *, store, createStore)();
			LOG_INFO("No configuration files found. Using an empty one");
		}

		checkFilesMerge(retReadOnlyConfig); // check once without the profile

		// apply profile to Store
		if(profilePath != NULL) {
			Store *profileConfig  = getStorePath(retReadOnlyConfig, "%s", profilePath);

			if(profileConfig ==  NULL) {
				LOG_ERROR("Given profile path does not exists: %s. Using empty read-only config", profilePath);
				$(void, store, freeStore)(retReadOnlyConfig);

				retReadOnlyConfig = $(Store *, store, createStore)();
			} else {
				retReadOnlyConfig = profileConfig;
			}

			checkFilesMerge(retReadOnlyConfig); // and once with the profile applied
		}

		/*
		 * So what we did until now: We loaded the read-only user specific
		 * configuration file and the global profiles configuration and got readOnlyConfig.
		 *
		 * The next step is to load the "gloabl config". The readOnlyConfig is merged
		 * into the global config Store without applying any profile path.
		 */

		if(g_file_test(globalConfigFilePath, G_FILE_TEST_EXISTS)) {
			Store *globalConfig = $(Store *, store, parseStoreFile)(globalConfigFilePath);

			if(!$(bool, store, mergeStore)(globalConfig, retReadOnlyConfig)) {
				LOG_WARNING("Could not merge global config Store into the read-only config");
			}

			$(void, store, freeStore)(retReadOnlyConfig);
			retReadOnlyConfig = globalConfig;
		}

		// clean up file path
		free(globalConfigFilePath);
	}

	return retReadOnlyConfig;
}

static Store *loadWritableConfig()
{
	Store *retWritableConfig = NULL;

	writableConfigFilePath = g_build_path("/", getUserKaliskoConfigPath(), USER_OVERRIDE_CONFIG_FILE_NAME, NULL);
	LOG_DEBUG("Expecting writable config at: %s", writableConfigFilePath);

	if(g_file_test(writableConfigFilePath, G_FILE_TEST_EXISTS)) {
		retWritableConfig = $(Store *, store, parseStoreFile)(writableConfigFilePath);
		writableConfigPathExists = true;
	} else {
		// check if the folder exists at least and if not we create one
		char *writableConfigDir = g_path_get_dirname(writableConfigFilePath);
		if(g_mkdir_with_parents(writableConfigDir, USER_CONFIG_DIR_PERMISSION) == -1) {
			writableConfigPathExists = false;
			LOG_DEBUG("The directory for the writable configuration file cannot be created. Saving will not work.");
		} else {
			LOG_INFO("Created directory for user specific configuration files at: %s", writableConfigDir);
			writableConfigPathExists = true;
		}
		free(writableConfigDir);
	}

	if(retWritableConfig == NULL) {
		retWritableConfig = $(Store *, store, createStore)();
	}

	return retWritableConfig;
}

static void checkFilesMerge(Store *store)
{
	Store *mergeDataStore = $(Store *, store, getStorePath)(store, MERGE_CONFIG_PATH);

	if(mergeDataStore != NULL) {
		if(mergeDataStore->type == STORE_STRING) {
			Store *storeToMerge = $(Store *, store, parseStoreFile)(mergeDataStore->content.string);

			if(storeToMerge != NULL) {
				checkFilesMerge(storeToMerge); // without a profile

				if(profilePath != NULL) {
					Store *storeToMergeProfiled = getStorePath(storeToMerge, "%s", profilePath);

					if(storeToMergeProfiled != NULL) {
						storeToMerge = storeToMergeProfiled;
					} else {
						LOG_INFO("Configuration to merge at '%s' has not the given profile. Ignoring", mergeDataStore->content.string);
						$(void, store, freeStore)(storeToMerge);
					}

					checkFilesMerge(storeToMerge); // with the profile
				}

				if(!$(bool, store, mergeStore)(store, storeToMerge)) {
					LOG_WARNING("Could not merge Store into the config: %s", mergeDataStore->content.string);
				}

				$(void, store, freeStore)(storeToMerge);
			} else {
				LOG_WARNING("Could not parse store file '%s' for configuration", mergeDataStore->content.string);
			}
		} else if(mergeDataStore->type == STORE_LIST) {
			GQueue *list = mergeDataStore->content.list;

			for(GList *iter = list->head; iter != NULL; iter = iter->next) {
				Store *filePath = (Store *)iter->data;

				if(filePath->type == STORE_STRING) {
					Store *storeToMerge = $(Store *, store, parseStoreFile)(filePath->content.string);

					if(storeToMerge == NULL) {
						LOG_WARNING("Could not parse store file '%s' for configuration", filePath->content.string);
						continue;
					}

					checkFilesMerge(storeToMerge); // without a profile

					if(profilePath != NULL) {
						Store *storeToMergeProfiled = getStorePath(storeToMerge, "%s", profilePath);

						if(storeToMergeProfiled != NULL) {
							storeToMerge = storeToMergeProfiled;
						} else {
							LOG_INFO("Configuration to merge at '%s' has not the given profile. Ignoring", mergeDataStore->content.string);
							$(void, store, freeStore)(storeToMerge);
							continue;
						}

						checkFilesMerge(storeToMerge); // with the profile
					}

					if(!$(bool, store, mergeStore)(store, storeToMerge)) {
						LOG_WARNING("Could not merge Store into the config: %s", mergeDataStore->content.string);
					}

					$(void, store, freeStore)(storeToMerge);
				} else {
					LOG_WARNING("Store merge list values must be strings representing file paths.");
				}
			}
		} else {
			LOG_WARNING("\"merge\" must be a string or a list of strings representing file path(s)");
		}
	}
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

	if(readOnlyConfig) {
		$(void, store, freeStore)(readOnlyConfig);
	}

}
