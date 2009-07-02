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

/**
 * A helper struct that's used to dump configs
 */
typedef struct {
	/** the config to dump */
	Config *config;
	/** the config writer to use for dumping */
	ConfigWriter *writer;
	/** the current indentation level */
	int level;
} ConfigDumpContext;

#define DUMP(FORMAT, ...) context->writer(context->config, FORMAT, ##__VA_ARGS__)

int yyparse(Config *config); // this can't go into a header because it doesn't have an API export

static void dumpConfig(Config *config, ConfigWriter *writer);
static void configFileWrite(Config *config, char *format, ...);
static void configGStringWrite(Config *config, char *format, ...);
static void dumpConfigSection(void *key, void *value, void *data);
static void dumpConfigNode(void *key_p, void *value_p, void *data);
static void dumpConfigNodeValue(ConfigNodeValue *value, ConfigDumpContext *context);
static GString *escapeConfigString(char *string);

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
	config->prelude = 0;

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
	config->prelude = 0;

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
 * Writes a config from memory to a file
 *
 * @param filename		the filename to write to
 * @param config		the config to write
 */
API void writeConfigFile(char *filename, Config *config)
{
	config->resource = fopen(filename, "w");
	dumpConfig(config, &configFileWrite);
	fclose(config->resource);
}

/**
 * Writes a config from memory to a GString
 *
 * @param config		the config to write
 * @result				the written config string, must be freed with g_string_free
 */
API GString *writeConfigGString(Config *config)
{
	config->resource = g_string_new("");
	dumpConfig(config, &configGStringWrite);
	return config->resource;
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
 * A ConfigWriter function for files
 *
 * @param config		the config to to write to
 * @param format		printf-like string to write
 */
static void configFileWrite(Config *config, char *format, ...)
{
	va_list va;

	va_start(va, format);

	vfprintf(config->resource, format, va);
}

/**
 * Dumps a parsed config from memory to a writer
 *
 * @param config		the config to dump
 * @param writer		the writer where the config should be dumped to
 */
static void dumpConfig(Config *config, ConfigWriter *writer)
{
	logInfo("Dumping config %s", config->name);

	ConfigDumpContext *context = allocateObject(ConfigDumpContext);
	context->config = config;
	context->writer = writer;
	context->level = 0;

	g_hash_table_foreach(config->sections, &dumpConfigSection, context);

	free(context);
}

/**
 * A ConfigWriter function for GStrings
 *
 * @param config		the config to to write to
 * @param format		printf-like string to write
 */
static void configGStringWrite(Config *config, char *format, ...)
{
	va_list va;

	va_start(va, format);

	g_string_append_vprintf(config->resource, format, va);
}

/**
 * A GHFunc to dump a config section
 *
 * @param key		the name of the section
 * @param value		the section's nodes
 * @param data		the dump's context
 */
static void dumpConfigSection(void *key, void *value, void *data)
{
	char *name = key;
	GHashTable *nodes = value;
	ConfigDumpContext *context = data;

	if(g_hash_table_size(nodes)) {
		DUMP("[%s]\n", name);
		g_hash_table_foreach(nodes, &dumpConfigNode, context);
	}
}

/**
 * A GHFunc to dump a config node
 *
 * @param key		the key of the node
 * @param value		the value of the node
 * @param data		the dump's context
 */
static void dumpConfigNode(void *key_p, void *value_p, void *data)
{
	char *key = key_p;
	ConfigNodeValue *value = value_p;
	ConfigDumpContext *context = data;

	for(int i = 0; i < context->level; i++) {
		DUMP("\t");
	}

	GString *escaped = escapeConfigString(key);

	DUMP("\"%s\" = ", escaped->str);

	g_string_free(escaped, TRUE);

	dumpConfigNodeValue(value, context);

	DUMP("\n");
}

/**
 * Dumps a config node value
 *
 * @param value		the value of the node
 * @param context		the dump's context
 */
static void dumpConfigNodeValue(ConfigNodeValue *value, ConfigDumpContext *context)
{
	GString *escaped;

	switch (value->type) {
		case CONFIG_STRING:
			escaped = escapeConfigString(value->content.string);

			DUMP("\"%s\"", escaped->str);

			g_string_free(escaped, TRUE);
		break;
		case CONFIG_INTEGER:
			DUMP("%d", value->content.integer);
		break;
		case CONFIG_FLOAT_NUMBER:
			DUMP("%f", value->content.float_number);
		break;
		case CONFIG_LIST:
			DUMP("(");

			for (GList *iter = value->content.list; iter != NULL; iter = iter->next) {
				dumpConfigNodeValue(iter->data, context);

				if(iter->next != NULL) {
					DUMP(", ");
				}
			}

			DUMP(")");
		break;
		case CONFIG_ARRAY:
			DUMP("{\n");
			context->level++;
			g_hash_table_foreach(value->content.array, &dumpConfigNode, context);
			context->level--;

			for(int i = 0; i < context->level; i++) {
				DUMP("\t");
			}

			DUMP("}");
		break;
	}
}

/**
 * Escapes a config string for output in a dump
 *
 * @param string		the string to escape
 * @result				the escaped string, must be freed with g_string_free
 */
static GString *escapeConfigString(char *string)
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
