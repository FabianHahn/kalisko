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

#ifndef CONFIG_PATH_H
#define CONFIG_PATH_H

/**
 * Enumeration of the config subtree types used for config path lookups
 */
typedef enum {
	/** Sections of the config */
	CONFIG_SECTIONS,
	/** Nodes of the config (in a section or an array) */
	CONFIG_NODES,
	/** A config string, integer or float value */
	CONFIG_LEAF_VALUE,
	/** Values of a config (in a list) */
	CONFIG_VALUES,
	/** An invalid location */
	CONFIG_NULL
} ConfigSubtreeType;

/**
 * A config subtree used for config path lookups
 */
typedef struct {
	/** Type of the subtree */
	ConfigSubtreeType type;
	/** The subtree */
	void *tree;
} ConfigSubtree;

API void *getConfigPathSubtree(Config *config, char *path);
API ConfigSubtreeType getConfigPathType(Config *config, char *path);
API bool setConfigPath(Config *config, char *path, void *value);
API bool deleteConfigPath(Config *config, char *path);
API GPtrArray *splitConfigPath(char *path);

#endif
