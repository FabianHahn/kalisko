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

#include <stdio.h>

#include "dll.h"
#include "memory_alloc.h"
#include "types.h"
#include "log.h"

#include "api.h"
#include "config.h"
#include "write.h"

/**
 * Convenience macro to write to config dumps
 *
 * @param FORMAT		printf-like format string
 */
#define DUMP(FORMAT, ...) context->writer(context->config, FORMAT, ##__VA_ARGS__)

static void configFileWrite(Config *config, char *format, ...);
static void configGStringWrite(Config *config, char *format, ...);
static void dumpConfig(Config *config, ConfigWriter *writer);
static void dumpConfigSection(void *key, void *value, void *data);
static void dumpConfigNode(void *key_p, void *value_p, void *data);
static void dumpConfigNodeValue(ConfigNodeValue *value, ConfigDumpContext *context);

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

			for (GList *iter = value->content.list->head; iter != NULL; iter = iter->next) {
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
