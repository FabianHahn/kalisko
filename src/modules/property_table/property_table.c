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
#include "dll.h"
#include "log.h"
#include "memory_alloc.h"
#include "api.h"
#include "property_table.h"

MODULE_NAME("property_table");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Allows to have in-memory HashTables based on a subject (void pointer)");
MODULE_VERSION(0, 0, 3);
MODULE_BCVERSION(0, 0, 1);
MODULE_NODEPS;

static void freeSubjectValue(void *value);

/**
 * HashTable that maps a void pointer to another GHashTable that maps from a string to a void pointer
 */
static GHashTable *subjects;

MODULE_INIT
{
	subjects = g_hash_table_new_full(&g_direct_hash, &g_direct_equal, NULL, freeSubjectValue);

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(subjects);
}

/**
 * Returns the value found in the table for the given subject and corresponding
 * to the given key.
 *
 * If no value exists NULL is returned.
 *
 * @param subject	The subject to find the corresponding subject specific table
 * @param key		The key to find the value in the table
 * @return NULL if no value was found otherwise the value
 */
API void *getPropertyTableValue(void *subject, char *key)
{
	GHashTable *table;

	if((table = g_hash_table_lookup(subjects, subject)) != NULL) {
		return g_hash_table_lookup(table, key);
	}

	return NULL;
}

/**
 * Sets, replaces or deletes the given value in the subject specific table using the given key.
 *
 * @param subject	The subject to find the corresponding subject specific table
 * @param key		The key to find the value in the table
 * @param value		The value to set or NULL if the key-value pair should be removed
 */
API void setPropertyTableValue(void *subject, char *key, void *value)
{
	GHashTable *table;

	// Get the table corresponding to the subject if it exists, else create one
	if((table = g_hash_table_lookup(subjects, subject)) == NULL) {
		table = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
		g_hash_table_insert(subjects, subject, table);
	}

	if(value == NULL) { // remove entry
		g_hash_table_remove(table, key);
	} else {
		g_hash_table_insert(table, strdup(key), value);
	}
}

/**
 * Frees the table corresponding to the given subject.
 *
 * @param subject	The subject for which the table has to be freed.
 */
API void freePropertyTable(void *subject)
{
	g_hash_table_remove(subjects, subject);
}

/**
 * Dumps all tables and their content into a string. It should only be used for
 * testing proposes.
 *
 * @return The dump as a string. This string must be freed.
 */
API char *dumpPropertyTables()
{
	GString *str = g_string_new("");

	GList *subjectList = g_hash_table_get_keys(subjects);
	for(GList *curSubject = subjectList; curSubject != NULL; curSubject = curSubject->next) {
		GHashTable *subjectTable = g_hash_table_lookup(subjects, curSubject->data);

		g_string_append_printf(str, "Subject: '%p'\n", curSubject->data);

		GList *keyList = g_hash_table_get_keys(subjectTable);
		for(GList *curKey = keyList; curKey != NULL; curKey = curKey->next) {
			g_string_append_printf(str, "\tKey: '%s' -> Value: '%p'\n", (char *)curKey->data, g_hash_table_lookup(subjectTable, curKey->data));
		}
	}

	char *retStr = str->str;
	g_string_free(str, false);

	return retStr;
}

static void freeSubjectValue(void *value)
{
	g_hash_table_destroy((GHashTable *) value);
}
