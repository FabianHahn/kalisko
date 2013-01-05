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

#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include "modules/store/store.h"


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
API Store *getConfig();

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
API Store *getWritableConfig();

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
API char *getProfilePath();


/**
 * @return The given path from the configuration Store.
 */
API Store *getConfigPath(char *path);

/**
 * Saves the writable store to the corresponding file.
 *
 * After saving the new writable config is merged together to a new global config.
 * So if you want to use the new settings in the writable config you must call this
 * function.
 *
 * At the end the event "savedWritableConfig" is triggered.
 */
API void saveWritableConfig();


/**
 * Reloads the configuration files (read-only and writable) and triggers at the end
 * the 'reloadedConfig' event to notify modules about the change.
 *
 * The writable one is a special case. This one is just loaded once as it is managed by
 * the application and there is no point to reload it.
 */
API void reloadConfig();

// ATTENTION: following functions are only for testing purposes

/**
 * <b>Only for testing</b>. Injects the given store as the read-only config Store.
 *
 * The new config is merged into a new read-only & writable config.
 *
 * @param new	The Store to inject
 * @return The old Store. Must be freed
 */
API Store* injectReadOnlyConfig(Store* new);

/**
 * <b>Only for testing</b>. Injects the given store as the writable config Store.
 *
 * The new config is merged into a new read-only & writable config.
 *
 * @param new				The Store to inject
 * @param updateConfig		Whether the config should be reloaded
 * @return The old Store. Must be freed
 */
API Store* injectWritableConfig(Store* new, bool updateConfig);

/**
 * <b>Only for testing</b>. Injects the given file path as the writable config file path.
 *
 * Does only change the paths. Config Stores are still the same after the call.
 *
 * @param filePath	The new file path to set. Must be allocated so the module can free it
 * @return The old file path. Must be freed
 */
API char* injectWritableConfigFilePath(char *filePath);

/**
 * <b>Only for testing</b>. Injects the given config profile path.
 *
 * @param profilePath	The new profile path to set. Must be allocated so the module can free it
 * @return The old profile path. Must be freed
 */
API char* injectConfigProfile(char *profilePath);

#endif
