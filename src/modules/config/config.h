/**
 * @file config.h
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

#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>
#include <stdio.h>

typedef enum {
	CONFIG_STRING,
	CONFIG_INTEGER,
	CONFIG_FLOAT_NUMBER,
	CONFIG_LIST,
	CONFIG_ARRAY
} ConfigNodeType;

typedef union {
	char *string;
	int integer;
	double float_number;
	GList *list;
	GHashTable *array;
} ConfigNodeValue;

typedef struct {
	char *key;
	ConfigNodeType type;
	ConfigNodeValue value;
} ConfigNode;

typedef struct {
	char *name;
	GHashTable *nodes;
} ConfigCategory;

typedef char (ConfigFileReader)(void *config); // first param has only type void * and not ConfigFile to get around C's single pass restrictions
typedef void (ConfigFileUnreader)(void *config, char c);

typedef struct ConfigFile {
	char *filename;
	GHashTable *categories;
	void *file;
	ConfigFileReader *read;
	ConfigFileUnreader *unread;
} ConfigFile;

typedef union {
	char *string;
	int integer;
	double float_number;
	ConfigNodeValue *value;
	ConfigNode *node;
	ConfigCategory *category;
} YYSTYPE;

#define CONFIG_MAX_STRING_LENGTH 1024

API char configFileRead(void *config);
API void configFileUnread(void *config, char c);
API char configStringRead(void *config);
API void configStringUnread(void *config, char c);

#endif
