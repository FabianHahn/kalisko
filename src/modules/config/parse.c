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
