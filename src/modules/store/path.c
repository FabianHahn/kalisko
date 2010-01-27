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

#include <assert.h>
#include <string.h>

#include "dll.h"
#include "memory_alloc.h"
#include "types.h"

#include "api.h"
#include "parse.h"
#include "path.h"

/**
 * Fetches a store value by its path
 *
 * @param store		the store in which the lookup takes place
 * @param path		the path to the value without a leading / to search, use integers from base 0 for list elements
 * @result				the store value, or NULL if not found
 */
API Store *getStorePath(Store *parent, char *path)
{
	if(strlen(path) == 0) { // empty path means the parent itself
		return parent;
	}

	GString *pathnode = g_string_new("");
	bool escaping = false;
	char *iter;

	for(iter = path; *iter != '\0'; iter++) { // Read until next unescaped slash or end
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
			g_string_free(pathnode, TRUE);
			return NULL;
		}

		g_string_append_c(pathnode, *iter);
	}

	Store *value;
	int i;

	switch(parent->type) {
		case STORE_ARRAY:
			value = g_hash_table_lookup(parent->content.array, pathnode->str);

			if(value == NULL) {
				g_string_free(pathnode, TRUE);
				return NULL;
			}
		break;
		case STORE_LIST:
			i = atoi(pathnode->str);

			if(i < 0 || i >= g_queue_get_length(parent->content.list)) { // out of bounds
				g_string_free(pathnode, TRUE);
				return NULL;
			} else {
				value = g_queue_peek_nth(parent->content.list, i);

				assert(value != NULL);
			}
		break;
		default:
			g_string_free(pathnode, TRUE);
			return NULL; // leaves don't have children
		break;
	}

	g_string_free(pathnode, TRUE);

	if(*iter == '\0') {
		return value;
	} else {
		return getStorePath(value, iter);
	}
}

/**
 * Sets a value in a store path
 *
 * @param store	the store to edit
 * @param path		the path to set, will be overridden if already exists
 * @param value		the value to set
 * @result			true if successful
 */
API bool setStorePath(Store *store, char *path, void *value)
{
	bool result = false;
	GPtrArray *array = splitStorePath(path);
	char **parts = (char **) array->pdata;
	char *key = parts[array->len - 1];
	parts[array->len - 1] = NULL; // cut off last element

	char *parentpath = g_strjoinv("/", parts);

	Store *parent = getStorePath(store, parentpath);

	int i;

	if(parent != NULL) {
		switch(parent->type) {
			case STORE_ARRAY:
				g_hash_table_insert(parent->content.array, key, value);
				result = true;
				// key doesn't need to be freed here since the hash table takes care of that
			break;
			case STORE_LIST:
				i = atoi(key);
				g_queue_push_nth(parent->content.list, value, i);
				result = true;
				free(key);
			break;
			default: // cannot write into a leaf value
				result = false;
				free(key);
			break;
		}
	} else {
		result = false;
	}

	// Cleanup
	free(parentpath);
	for(i = 0; i < array->len - 1; i++) {
		free(parts[i]);
	}
	g_ptr_array_free(array, TRUE);

	return result;
}

/**
 * Deletes a value in a store path
 *
 * @param store	the store to edit
 * @param path		the path to delete
 * @result			true if successful
 */
API bool deleteStorePath(Store *store, char *path)
{
	bool result = false;
	GPtrArray *array = splitStorePath(path);
	char **parts = (char **) array->pdata;
	char *key = parts[array->len - 1];
	parts[array->len - 1] = NULL; // cut off last element

	char *parentpath = g_strjoinv("/", parts);

	Store *parent = getStorePath(store, parentpath);

	int i;
	void *value;

	if(parent != NULL) {
		switch(parent->type) {
			case STORE_ARRAY:
				result = g_hash_table_remove(parent->content.array, key); // The hash table frees the removed value automatically
			break;
			case STORE_LIST:
				i = atoi(key);
				value = g_queue_pop_nth(parent->content.list, i);
				if(value != NULL) {
					freeStore(value);
					result = true;
				} else {
					result = false;
				}
			break;
			default: // cannot delete inside a leaf value
				result = false;
			break;
		}
	} else {
		result = false;
	}

	// Cleanup
	free(parentpath);
	free(key);
	for(i = 0; i < array->len - 1; i++) {
		free(parts[i]);
	}
	g_ptr_array_free(array, TRUE);

	return result;
}

/**
 * Splits a store path by its unescaped delimiter '/'
 *
 * @param path		the path to escape
 * @result			an array of path elements, contents must be freed with free and the array itself with g_ptr_array_free
 */
API GPtrArray *splitStorePath(char *path)
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
