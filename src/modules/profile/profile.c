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
#include "log.h"
#include "module.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "api.h"

MODULE_NAME("profile");
MODULE_AUTHOR("The Kalisko Team");
MODULE_DESCRIPTION("The profile module allows you to define sets of modules that belong together and load them together");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0));

MODULE_INIT
{
	char *profile;

	Store *profiles = $(Store *, config, getConfigPathValue)("profiles");

	if(profiles == NULL || store->type != STORE_ARRAY) {
		LOG_ERROR("Failed to load profiles config section");
		return false;
	}

	Store *profileset;
	if((profileset = g_hash_table_lookup(profiles->content.array, profile)) == NULL) {
		LOG_ERROR("Couldn't find requested profile '%s' in config profiles section", profile);
		return false;
	}

	if(profileset->content.array != STORE_LIST) {
		LOG_ERROR("Profile config section '%s' is not a list", profile);
		return false;
	}

	LOG_INFO("Loading profile '%s'", profile);
	for(GList *iter = profileset->content.list; iter != NULL; iter = iter->next) {
		Store *entry = iter->data;

		if(entry->type != STORE_STRING) {
			LOG_WARNING("Encountered non-string profile entry, skipping");
			continue;
		}

		char *module = entry->content.string;

		if(!$$(bool, requestModule)(module)) {
			LOG_ERROR("Requesting module '%s' for profile '%s' failed, aborting profile loading", module, profile);
			return false;
		}
	}

	return true;
}

MODULE_FINALIZE
{

}
