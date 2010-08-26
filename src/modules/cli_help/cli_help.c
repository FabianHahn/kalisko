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


#include <glib.h>
#include <stdio.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/getopts/getopts.h"

#include "api.h"
#include "cli_help.h"


MODULE_NAME("cli_help");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Allows to show a command line help.");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("getopts", 0, 1, 0));

typedef struct {
	char *module;
	char *shortOpt;
	char *longOpt;
	char *briefHelp;
	char *longHelp;
} CLOption;

HOOK_LISTENER(modules_loaded);
inline static char *xstrdup(const char *str);
inline static void xfree(void *obj);

#define SHORT_OPT_PREFIX "-"
#define LONG_OPT_PREFIX "--"
#define OPT_SEPERATOR ", "

static GSList *clOptions;
static int maxShortOptLength;
static int maxLongOptLength;

MODULE_INIT
{
	clOptions = NULL;
	maxShortOptLength = 0;
	maxLongOptLength = 0;

	if(!HOOK_ATTACH(module_perform_finished, modules_loaded)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(module_perform_finished, modules_loaded);

	for(GSList *current = clOptions; current != NULL; current = g_slist_next(current)) {
		CLOption *option = (CLOption *) current->data;
		xfree(option->briefHelp);
		xfree(option->longHelp);
		xfree(option->longOpt);
		xfree(option->module);
		xfree(option->shortOpt);

		free(option);
	}

	g_slist_free(clOptions);
}

HOOK_LISTENER(modules_loaded)
{
	if(!$(char *, getopts, getOpt)("h") && !$(char *, getopts, getOpt)("help")) {
		return;
	}

	char *execName = $$(char *, getExecutableName)();
	printf("\n%s%s%s\n\n", "Usage: ", execName, " [options] [arguments]");
	free(execName);

	clOptions = g_slist_reverse(clOptions);
	GSList *current = clOptions;
	if(current) {
		printf("%s\n", "Options:");
	} else {
		printf("%s\n", "No option help found.");
	}

	for(/* already initialized */; current != NULL; current = g_slist_next(current)) {
		CLOption *option = (CLOption *)current->data;

		// Print shortOpt & longOpt
		if(option->shortOpt) {
			printf("%s%s", SHORT_OPT_PREFIX, option->shortOpt);
		}

		if(!option->shortOpt && option->longOpt) {
			printf("%s%s", LONG_OPT_PREFIX, option->longOpt);
		}

		if(option->longOpt && option->shortOpt) {
			printf("%s%s%s", OPT_SEPERATOR, LONG_OPT_PREFIX, option->longOpt);
		}

		// Print brief help & module name

		// Amount of whitespaces between the short/long option and the help text so all help text are one below the other
		int whitespaces = maxShortOptLength + maxLongOptLength + strlen(OPT_SEPERATOR) + strlen(SHORT_OPT_PREFIX) + strlen(LONG_OPT_PREFIX) + 4 \
		- (option->shortOpt ? (g_utf8_strlen(option->shortOpt, -1) + strlen(SHORT_OPT_PREFIX)) : 0) \
		- (option->longOpt ? (g_utf8_strlen(option->longOpt, -1) + strlen(LONG_OPT_PREFIX)) : 0) \
		- (option->longOpt && option->shortOpt ? strlen(OPT_SEPERATOR) : 0);

		char *whitepace = g_strnfill(whitespaces, ' '); // Change the whitespace char (' ') to a dot ('.') and you see where the whitespaces are used ;-)

		printf("%s%s (Module: %s)\n", whitepace, option->briefHelp, option->module);

		free(whitepace);
	}
}

/**
 * Adds a new help entry for the given short / long command line option.
 *
 * Altough shortOpt and longOpt are optional one of them must be given.
 * All strings must be \0 terminated and encoded in UTF-8 (and because of that
 * also ASCII is allowed).
 *
 * @param moduleName	The name of the module owning the option. Required.
 * @param shortOpt		The short option without a prepended dash (so without the '-'). Optional.
 * @param longOpt		The long option without a prepended double dash (so without the '--'). Optional.
 * @param briefHelp		The short help. Required.
 * @param longHelp		The long help explaining how to use this specific option. Optional.
 * @return True if the help was added, false on error.
 */
API bool addCLOptionHelp(char *moduleName, char *shortOpt, char *longOpt, char *briefHelp, char *longHelp)
{
	// Check params
	if(!moduleName) {
		LOG_WARNING("CLI option owner module name must be given to add a new CLI help entry.");
		return false;
	}

	if(!briefHelp) {
		LOG_WARNING("Brief help for CLI option help entry must be given.");
		return false;
	}

	if(!(shortOpt || longOpt)) {
		LOG_WARNING("Short option or long option must be given to add a new CLI help entry.");
		return false;
	}

	// Create entry
	CLOption *option = ALLOCATE_OBJECT(CLOption);
	option->module = xstrdup(moduleName);
	option->shortOpt = xstrdup(shortOpt);
	option->longOpt = xstrdup(longOpt);
	option->briefHelp = xstrdup(briefHelp);
	option->longHelp = xstrdup(longHelp);

	clOptions = g_slist_prepend(clOptions, option);

	// Check length of shortOpt and longOpt to create a nice output later
	int shortOptLength = g_utf8_strlen(shortOpt, -1);
	if(maxShortOptLength < shortOptLength) {
		maxShortOptLength = shortOptLength;
	}

	int longOptLength = g_utf8_strlen(longOpt, -1);
	if(maxLongOptLength < longOptLength) {
		maxLongOptLength = longOptLength;
	}

	return true;
}

inline static char *xstrdup(const char *str)
{
	return str ? strdup(str) : NULL;
}

inline static void xfree(void *obj)
{
	obj ? free(obj) : NULL;
}
