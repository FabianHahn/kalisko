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
#include <glib.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "timer.h"
#include "util.h"

#include "api.h"

MODULE_NAME("getopts");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module parses command line arguments and stores them for later use.");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

static GHashTable *opts;
static bool parsed;

MODULE_INIT
{
	opts = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
	parsed = false;
	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(opts);
}

void parseArgv()
{
	char **argv = getArgv();
	int argc = getArgc();

	for(int i = 0; i < argc; i++) {
		// Long option
		if(g_str_has_prefix(argv[i], "--")) {
			// End of option string when this is just "--"
			if(strlen(argv[i]) == 2) {
				break;
			}
			// continue when there are more than two dashes
			if(argv[i][2] == '-') {
				continue;
			}
			char *opt = &argv[i][2];
			// find equal sign
			char *equal = g_strstr_len(opt, -1, "=");
			// Store option
			if(equal == NULL) {
				g_hash_table_insert(opts, g_strdup(opt), g_strdup(""));
			}
			else {
				g_hash_table_insert(opts, g_strndup(opt, equal - opt), g_strdup(equal + 1));
			}
		}
		// Short option(s)
		else if(*argv[i] == '-') {
			// There can't be any more dashes due to the previous condition, so simply skip the first
			// Anything thereafter is the name of the short option
			char *key = g_strdup(&argv[i][1]);
			char *elem = "";
			// If the following argument is NOT an option, take it as this option's param
			if(argv[i+1] != NULL && argv[i+1][0] != '-') {
				elem = argv[i+1];
				i++;
			}
			g_hash_table_insert(opts, key, g_strdup(elem));
		}
		else {
			// This means a token has been supplied that neither is an option nor belongs to one
		}
	}

	parsed = true;
}

/**
 * Looks up an option and returns its supplied value if any or an empty string if the option was supplied without value.
 * Returns NULL if the option was not supplied at all.
 *
 * @param opt	The option to look up
 * @return		The value for the given option or an empty string or NULL
 */
API char *getOpt(char *opt)
{
	// Have the arguments been parsed yet?
	if(!parsed) {
		parseArgv();
	}
	// Return item from hashtable
	return (char*)g_hash_table_lookup(opts, opt);
}
