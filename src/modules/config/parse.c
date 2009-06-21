/**
 * @file parse.c
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

#include "dll.h"
#include "memory_alloc.h"
#include "types.h"
#include "log.h"

#include "api.h"
#include "parse.h"
#include "parser.h"
#include "lexer.h"

int yyparse(Config *config); // this can't go into a header because it doesn't have an API export

static void dumpConfigSection(void *key, void *value, void *data);
static void dumpConfigNode(void *key_p, void *value_p, void *data);
static void dumpConfigNodeValue(char *key, ConfigNodeValue *value, GString *dump);

/**
 * Parses a config file
 *
 * @param filename		the file name of the config file to parse
 * @result				the parsed config
 */
API Config *parseConfigFile(char *filename)
{
	Config *config = allocateObject(Config);

	config->name = strdup(filename);
	config->resource = fopen(config->name, "r");
	config->read = &configFileRead;
	config->unread = &configFileUnread;

	logInfo("Parsing config file %s", config->name);

	if (yyparse(config) != 0) {
		logError("Parsing config file %s failed", config->name);
		free(config->name);
		fclose(config->resource);
		free(config);
		return NULL;
	}

	fclose(config->resource);

	return config;
}

/**
 * Parses a config string
 *
 * @param filename		the config string to parse
 * @result				the parsed config
 */
API Config *parseConfigString(char *string)
{
	Config *config = allocateObject(Config);

	config->name = strdup(string);
	config->resource = config->name;
	config->read = &configStringRead;
	config->unread = &configStringUnread;

	logInfo("Parsing config string: %s", config->name);

	if (yyparse(config) != 0) {
		logError("Parsing config string failed");
		free(config->name);
		free(config);
		return NULL;
	}

	return config;
}

/**
 * Frees a parsed config
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
			for (GList *iter = value->content.list; iter != NULL; iter = iter->next) {
				freeConfigNodeValue(iter->data);
			}

			g_list_free(value->content.list);
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
 * Dumps a parsed config
 *
 * @param config		the config to dump
 * @result				the config's dump as a string, must be freed with g_string_free afterwards
 */
API GString *dumpConfig(Config *config)
{
	GString *dump = g_string_new("");
	g_string_append_printf(dump, "Config dump for %s:\n", config->name);

	g_hash_table_foreach(config->sections, &dumpConfigSection, dump);

	return dump;
}

/**
 * A ConfigReader function for files
 *
 * @param config		the config to to read from
 * @result				the read character
 */
API char configFileRead(void *config)
{
	Config *cfg = config;

	return fgetc(cfg->resource);
}

/**
 * A ConfigUnreader function for files
 *
 * @param config		the config to to unread to
 * @param c				the character to unread
 */
API void configFileUnread(void *config, char c)
{
	Config *cfg = config;

	ungetc(c, cfg->resource);
}

/**
 * A ConfigReader function for strings
 *
 * @param config		the config to to read from
 * @result				the read character
 */
API char configStringRead(void *config)
{
	Config *cfg = config;

	return *((char *) cfg->resource++);
}

/**
 * A ConfigUnreader function for strings
 *
 * @param config		the config to to unread to
 * @param c				the character to unread
 */
API void configStringUnread(void *config, char c)
{
	Config *cfg = config;

	cfg->resource--;
}

/**
 * A GHFunc to dump a config section
 *
 * @param key		the name of the section
 * @param value		the section's nodes
 * @param dump		the dump target string
 */
static void dumpConfigSection(void *key, void *value, void *data)
{
	char *name = key;
	GHashTable *nodes = value;
	GString *dump = data;

	g_string_append_printf(dump, "Section \"%s\":\n", name);
	g_hash_table_foreach(nodes, &dumpConfigNode, dump);
}

/**
 * A GHFunc to dump a config node
 *
 * @param key		the key of the node
 * @param value		the value of the node
 * @param dump		the dump target string
 */
static void dumpConfigNode(void *key_p, void *value_p, void *data)
{
	char *key = key_p;
	ConfigNodeValue *value = value_p;
	GString *dump = data;

	GString *quoted_key = g_string_new("");
	g_string_append_printf(quoted_key, "\"%s\"", key);
	dumpConfigNodeValue(quoted_key->str, value, dump);
	g_string_free(quoted_key, TRUE);
}

/**
 * Dumps a config node value
 *
 * @param key		the key of the parent node
 * @param value		the value of the node
 * @param dump		the dump target string
 */
static void dumpConfigNodeValue(char *key, ConfigNodeValue *value, GString *dump)
{
	switch (value->type) {
		case CONFIG_STRING:
			g_string_append_printf(dump, "String %s: \"%s\"\n", key,
			value->content.string);
		break;
		case CONFIG_INTEGER:
			g_string_append_printf(dump, "Integer %s: %d\n", key,
			value->content.integer);
		break;
		case CONFIG_FLOAT_NUMBER:
			g_string_append_printf(dump, "Float %s: %f\n", key,
			value->content.float_number);
		break;
		case CONFIG_LIST:
			g_string_append_printf(dump, "List %s:\n", key);

			for (GList *iter = value->content.list; iter != NULL; iter = iter->next) {
				dumpConfigNodeValue("[list item]", iter->data, dump);
			}
		break;
		case CONFIG_ARRAY:
			g_string_append_printf(dump, "Array %s:\n", key);
			g_hash_table_foreach(value->content.array, &dumpConfigNode, dump);
			g_string_append_printf(dump, "End of array %s\n", key);
		break;
	}
}
