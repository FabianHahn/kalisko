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

#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include <glib.h>

/**
 * Enumeration of the four standard log levels.
 */
typedef enum {
	/** A string value */
	CONFIG_STRING,
	/** An integer value */
	CONFIG_INTEGER,
	/** A floating point number value */
	CONFIG_FLOAT_NUMBER,
	/** A list value */
	CONFIG_LIST,
	/** An associative array value */
	CONFIG_ARRAY
} ConfigValueType;

/**
 * Union to store a node value's content
 */
typedef union {
	/** A string value */
	char *string;
	/** An integer value */
	int integer;
	/** A floating point number value */
	double float_number;
	/** A list value */
	GQueue *list;
	/** An associative array value */
	GHashTable *array;
} ConfigNodeValueContent;

/**
 * Struct to represent a node value
 */
typedef struct {
	/** The node value's type */
	ConfigValueType type;
	/** The node value's content */
	ConfigNodeValueContent content;
} ConfigNodeValue;

/**
 * Struct to represent a config node
 * Note: This struct is only used internally to do the parsing, it is NOT used in the final parsed config's representation
 */
typedef struct {
	/** The node's key */
	char *key;
	/** The node's value */
	ConfigNodeValue *value;
} ConfigNode;

/**
 * Struct to represent a config section
 * Note: This struct is only used internally to do the parsing, it is NOT used in the final parsed config's representation
 */
typedef struct {
	/** The section's name */
	char *name;
	/** The section's nodes */
	GHashTable *nodes;
} ConfigSection;

/**
 * A config reader to retrieve characters from a source
 * Note: The first param has only type void * and not ConfigFile to get around C's single pass compilation restrictions
 */
typedef char (ConfigReader)(void *config);

/**
 * A config unreader to push back characters into a source
 * Note: The first param has only type void * and not ConfigFile to get around C's single pass compilation restrictions
 */
typedef void (ConfigUnreader)(void *config, char c);

/**
 * Struct to represent a config
 */
typedef struct {
	/** The config's identification name */
	char *name;
	/** The config's resource */
	void *resource;
	/** The config's reader */
	ConfigReader *read;
	/** The config's unreader */
	ConfigUnreader *unread;
	/** How many parts of the prelude were already sent */
	int prelude;
	/** The config's sections if it's parsed */
	GHashTable *sections;
} Config;

API Config *createConfig(char *name);
API void freeConfig(Config *config);
API void freeConfigNodeValue(void *value);
API void *getConfigValueContent(ConfigNodeValue *value);
API GString *escapeConfigString(char *string);
API ConfigNodeValue *createConfigStringValue(char *string);
API ConfigNodeValue *createConfigIntegerValue(int integer);
API ConfigNodeValue *createConfigFloatNumberValue(double float_number);
API ConfigNodeValue *createConfigListValue(GQueue *list);
API ConfigNodeValue *createConfigArrayValue(GHashTable *array);
API GHashTable *createConfigNodes();

#endif
