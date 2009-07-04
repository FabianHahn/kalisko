/**
 * @file path.c
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

#include <string.h>

#include "dll.h"
#include "memory_alloc.h"
#include "types.h"

#include "api.h"
#include "parse.h"
#include "path.h"

static void *getConfigSubpath(char *subpath, ConfigSubtreeType type, void *parent, bool fetch_value);

/**
 * Fetches a config subtree by its path
 *
 * @param config		the config in which the lookup takes place
 * @param path			the path to the subtree without a leading / to search, use integers from base 0 for list elements
 * @result				the config subtree, or NULL if not found
 */
API void *getConfigPathSubtree(Config *config, char *path)
{
	if(strlen(path) == 0) { // empty path means the sections subtree itself
		return config->sections;
	} else {
		return getConfigSubpath(path, CONFIG_SECTIONS, config->sections, true);
	}
}

/**
 * Looks up the type of a config subtree
 *
 * @param config		the config in which the lookup takes place
 * @param path			the path to the subtree without a leading / to search, use integers from base 0 for list elements
 * @result				the config subtree's type
 */
API ConfigSubtreeType getConfigPathType(Config *config, char *path)
{
	if(strlen(path) == 0) { // empty path means the sections subtree itself
		return CONFIG_SECTIONS;
	} else {
		ConfigSubtreeType *type = getConfigSubpath(path, CONFIG_SECTIONS, config->sections, false);
		ConfigSubtreeType ret = *type;
		free(type);
		return ret;
	}
}

/**
 * Sets a value in a config tree
 *
 * @param config	the config to edit
 * @param path		the path to set, will be overridden if already exists
 * @param value		the value to set
 * @result			true if successful
 */
API bool setConfigPath(Config *config, char *path, void *value)
{
	bool result = false;
	GPtrArray *array = splitConfigPath(path);
	char **parts = (char **) array->pdata;
	char *key = parts[array->len - 1];
	parts[array->len - 1] = NULL; // cut off last element

	char *parentpath = g_strjoinv("/", parts);

	ConfigSubtreeType type = getConfigPathType(config, parentpath);

	GHashTable *table;
	int i;

	switch(type) {
		case CONFIG_SECTIONS:
		case CONFIG_NODES:
			table = getConfigPathSubtree(config, parentpath);
			g_hash_table_insert(table, key, value);
			result = true;
		break;
		case CONFIG_VALUES:
			i = atoi(key);
			g_queue_push_nth(getConfigPathSubtree(config, parentpath), value, i);
			result = true;
			free(key);
		break;
		case CONFIG_LEAF_VALUE: // cannot write into a leaf value
		case CONFIG_NULL:
			result = false;
			free(key);
		break;
	}

	// Cleanup
	free(parentpath);
	for(int i = 0; i < array->len - 1; i++) {
		free(parts[i]);
	}
	g_ptr_array_free(array, TRUE);

	return result;
}

/**
 * Deletes a value in a config tree
 *
 * @param config	the config to edit
 * @param path		the path to set, will be overridden if already exists
 * @result			true if successful
 */
API bool deleteConfigPath(Config *config, char *path)
{
	bool result = false;
	GPtrArray *array = splitConfigPath(path);
	char **parts = (char **) array->pdata;
	char *key = parts[array->len - 1];
	parts[array->len - 1] = NULL; // cut off last element

	char *parentpath = g_strjoinv("/", parts);

	ConfigSubtreeType type = getConfigPathType(config, parentpath);

	GHashTable *table;
	int i;
	void *value;

	switch(type) {
		case CONFIG_SECTIONS:
		case CONFIG_NODES:
			table = getConfigPathSubtree(config, parentpath);
			g_hash_table_remove(table, key);
			result = true;
		break;
		case CONFIG_VALUES:
			i = atoi(key);
			value = g_queue_pop_nth(getConfigPathSubtree(config, parentpath), i);
			if(value != NULL) {
				freeConfigNodeValue(value);
				result = true;
			} else {
				result = false;
			}
		break;
		case CONFIG_LEAF_VALUE: // cannot write into a leaf value
		case CONFIG_NULL:
			result = false;
		break;
	}

	// Cleanup
	free(parentpath);
	free(key);
	for(int i = 0; i < array->len - 1; i++) {
		free(parts[i]);
	}
	g_ptr_array_free(array, TRUE);

	return result;
}

/**
 * Splits a config path by its unescaped delimiter '/'
 *
 * @param path		the path to escape
 * @result			an array of path elements, contents must be freed with free and the array itself with g_ptr_array_free
 */
API GPtrArray *splitConfigPath(char *path)
{
	GPtrArray *array = g_ptr_array_new();
	GString *assemble = g_string_new("");
	char *iter;
	bool escaping = false;

	for(iter = path; *iter != '\0'; iter++) {
		if(*iter == '\\') {
			if(!escaping) {
				escaping = true;
				continue;
			} else {
				escaping = false;
			}
		} else if(*iter == '/') {
			if(!escaping) {
				g_ptr_array_add(array, assemble->str);
				g_string_free(assemble, FALSE);
				assemble = g_string_new("");
				continue;
			} else {
				escaping = false;
			}
		}

		if(escaping) { // still escaping, this should not happen
			// Cleanup
			for(int i = 0; i < array->len; i++) {
				free(array->pdata[i]);
			}
			g_ptr_array_free(array, TRUE);
			g_string_free(assemble, TRUE);
			return NULL;
		}

		g_string_append_c(assemble, *iter);
	}

	g_ptr_array_add(array, assemble->str);
	g_string_free(assemble, FALSE);

	return array;
}

/**
 * Fetches a config subtree by its parent tree and its subpath
 *
 * @see getConfigPath
 * @param subpath		the subpath to search
 * @param type			the type of the parent
 * @param parent		the parent config tree to search
 * @param fetch_value	true if the value should be fetched, false for the type
 * @result				the config subtree / type depending on fetch_value, or NULL / CONFIG_NULL if not found
 */
static void *getConfigSubpath(char *subpath, ConfigSubtreeType type, void *parent, bool fetch_value)
{
	ConfigSubtreeType subtype = CONFIG_NULL;
	void *subtree = NULL;
	GString *pathnode = g_string_new("");
	bool escaping = false;
	char *iter;

	for(iter = subpath; *iter != '\0'; iter++) {
		if(*iter == '/') {
			if(!escaping) {
				iter++;
				break;
			} else {
				escaping = false;
			}
		} else if(*iter == '\\') {
			if(!escaping) {
				escaping = true;
				continue;
			} else {
				escaping = false;
			}
		}

		if(escaping) { // we're still escaping, that's not possible
			if(fetch_value) {
				return NULL;
			} else {
				ConfigSubtreeType *ret = allocateObject(ConfigSubtreeType);
				*ret = CONFIG_NULL;
				return ret;
			}
		}

		g_string_append_c(pathnode, *iter);
	}

	GHashTable *table;
	ConfigNodeValue *value;
	int i;

	switch(type) {
		case CONFIG_SECTIONS:
			table = g_hash_table_lookup(parent, pathnode->str);

			if(table == NULL) {
				subtype = CONFIG_NULL;
				subtree = NULL;
			} else {
				subtype = CONFIG_NODES;
				subtree = table;
			}
		break;
		case CONFIG_NODES:
			value = g_hash_table_lookup(parent, pathnode->str);

			if(value == NULL) {
				subtype = CONFIG_NULL;
				subtree = NULL;
			} else {
				switch(value->type) {
					case CONFIG_ARRAY:
						subtype = CONFIG_NODES;
						subtree = value->content.array;
					break;
					case CONFIG_LIST:
						subtype = CONFIG_VALUES;
						subtree = value->content.list;
					break;
					default:
						subtype = CONFIG_LEAF_VALUE;
						subtree = value;
					break;
				}
			}
		break;
		case CONFIG_LEAF_VALUE:
			subtype = CONFIG_NULL; // leaves don't have children
			subtree = NULL;
		break;
		case CONFIG_VALUES:
			i = atoi(pathnode->str);

			if(i >= g_queue_get_length(parent)) { // out of bounds
				subtype = CONFIG_NULL;
				subtree = NULL;
			} else {
				value = g_queue_peek_nth(parent, i);

				switch(value->type) {
					case CONFIG_ARRAY:
						subtype = CONFIG_NODES;
						subtree = value->content.array;
					break;
					case CONFIG_LIST:
						subtype = CONFIG_VALUES;
						subtree = value->content.list;
					break;
					default:
						subtype = CONFIG_LEAF_VALUE;
						subtree = value;
					break;
				}
			}
		break;
		case CONFIG_NULL:
			subtype = CONFIG_NULL;
			subtree = NULL;
		break;
	}

	if(*iter == '\0') {
		if(fetch_value) {
			return subtree;
		} else {
			ConfigSubtreeType *ret = allocateObject(ConfigSubtreeType);
			*ret = subtype;
			return ret;
		}
	} else {
		return getConfigSubpath(iter, subtype, subtree, fetch_value);
	}
}
