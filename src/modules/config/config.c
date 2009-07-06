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
#include <string.h>

#include "dll.h"
#include "types.h"
#include "memory_alloc.h"
#include "util.h"

#include "api.h"
#include "config.h"

API bool module_init()
{
	return true;
}

API void module_finalize()
{

}

API GList *module_depends()
{
	return NULL;
}

/**
 * Creates an empty config
 *
 * @param name		the name for the new config
 * @result			the created config
 */
API Config *createConfig(char *name)
{
	Config *config = allocateObject(Config);

	config->name = strdup(name);
	config->resource = NULL;
	config->read = NULL;
	config->unread = NULL;
	config->prelude = 0;
	config->sections = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &destroyGHashTable);

	return config;
}

/**
 * Frees a config
 *
 * @param config		the config to free
 */
API void freeConfig(Config *config)
{
	g_hash_table_destroy(config->sections);
	free(config->name);

	free(config);
}

/**
 * A GDestroyNotify function to free a config node value
 *
 * @param config		the config node value to free
 */
API void freeConfigNodeValue(void *value_p)
{
	ConfigNodeValue *value = value_p;

	switch (value->type) {
		case CONFIG_STRING:
			free(value->content.string);
		break;
		case CONFIG_LIST:
			for(GList *iter = value->content.list->head; iter != NULL; iter = iter->next) {
				freeConfigNodeValue(iter->data);
			}

			g_queue_free(value->content.list);
		break;
		case CONFIG_ARRAY:
			g_hash_table_destroy(value->content.array);
		break;
		default:
			// No need to free ints or doubles
		break;
	}

	free(value);
}


/**
 * Returns a node value's actual content
 *
 * @param value		the config node value
 * @result 			the node value's content
 */
API void *getConfigValueContent(ConfigNodeValue *value)
{
	switch(value->type) {
		case CONFIG_STRING:
			return value->content.string;
		break;
		case CONFIG_LIST:
			return value->content.list;
		break;
		case CONFIG_ARRAY:
			return value->content.array;
		break;
		default:
			return &value->content;
		break;
	}
}

/**
 * Escapes a config string for output in a dump
 *
 * @param string		the string to escape
 * @result				the escaped string, must be freed with g_string_free
 */
API GString *escapeConfigString(char *string)
{
	GString *escaped = g_string_new("");

	for(char *iter = string; *iter != '\0'; iter++) {
		if(*iter == '"' || *iter == '\\') {
			g_string_append_printf(escaped, "\\%c", *iter); // escape the character
		} else {
			g_string_append_c(escaped, *iter);
		}
	}

	return escaped;
}

/**
 * Creates a string value to be used in a config
 *
 * @param string		the content string
 * @result				the created node value, must be freed with freeConfigNodeValue or by the config system
 */
API ConfigNodeValue *createConfigStringValue(char *string)
{
	ConfigNodeValue *value = allocateObject(ConfigNodeValue);
	value->type = CONFIG_STRING;
	value->content.string = strdup(string);

	return value;
}

/**
 * Creates an integer value to be used in a config
 *
 * @param string		the content integer
 * @result				the created node value, must be freed with freeConfigNodeValue or by the config system
 */
API ConfigNodeValue *createConfigIntegerValue(int integer)
{
	ConfigNodeValue *value = allocateObject(ConfigNodeValue);
	value->type = CONFIG_INTEGER;
	value->content.integer = integer;

	return value;
}

/**
 * Creates a float number value to be used in a config
 *
 * @param string		the content float number
 * @result				the created node value, must be freed with freeConfigNodeValue or by the config system
 */
API ConfigNodeValue *createConfigFloatNumberValue(double float_number)
{
	ConfigNodeValue *value = allocateObject(ConfigNodeValue);
	value->type = CONFIG_FLOAT_NUMBER;
	value->content.float_number = float_number;

	return value;
}

/**
 * Creates a list value to be used in a config
 *
 * @param string		the content list or NULL for an empty list
 * @result				the created node value, must be freed with freeConfigNodeValue or by the config system
 */
API ConfigNodeValue *createConfigListValue(GQueue *list)
{
	ConfigNodeValue *value = allocateObject(ConfigNodeValue);
	value->type = CONFIG_LIST;

	if(list != NULL) {
		value->content.list = list;
	} else {
		value->content.list = g_queue_new();
	}

	return value;
}

/**
 * Creates an array value to be used in a config
 *
 * @param string		the content array
 * @result				the created node value, must be freed with freeConfigNodeValue or by the config system
 */
API ConfigNodeValue *createConfigArrayValue(GHashTable *array)
{
	ConfigNodeValue *value = allocateObject(ConfigNodeValue);
	value->type = CONFIG_ARRAY;

	if(array != NULL) {
		value->content.array = array;
	} else {
		value->content.array = createConfigNodes();
	}

	return value;
}

/**
 * Creates an empty config nodes table to be used as a section or an array in a config
 *
 * @result			the created nodes table, must be freed with g_hash_table_destroy or the config system
 */
API GHashTable *createConfigNodes()
{
	return g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeConfigNodeValue);
}
