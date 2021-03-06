/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, 2011, Kalisko Project Leaders
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
#include "log.h"
#include "types.h"
#include "util.h"
#include "memory_alloc.h"
#include "modules/getopts/getopts.h"
#include "modules/table/table.h"
#include "modules/plaintext_table/plaintext_table.h"
#include "modules/event/event.h"

#define API
#include "cli_help.h"


MODULE_NAME("cli_help");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Allows to show a command line help.");
MODULE_VERSION(0, 2, 5);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("getopts", 0, 1, 0), MODULE_DEPENDENCY("plaintext_table", 0, 1, 2), MODULE_DEPENDENCY("table", 0, 1, 5), MODULE_DEPENDENCY("event", 0, 1, 1));

typedef struct {
	char *module;
	char *shortOpt;
	char *longOpt;
	char *briefHelp;
} CLOption;

typedef struct {
	char *module;
	char *name;
	char *briefHelp;
} CLArgument;

static void listener_modulesLoaded(void *subject, const char *event, void *data, va_list args);
static void printOptionsHelp(Table *table);
static void printArgumentHelp(Table *table, bool isAfterOptions);

#define SHORT_OPT_PREFIX "-"
#define LONG_OPT_PREFIX "--"
#define OPT_SEPERATOR ", "

static GSList *clOptions;
static GSList *clArguments;
static bool hasOptions;
static bool hasArguments;

MODULE_INIT
{
	clOptions = NULL;
	clArguments = NULL;
	hasOptions = false;
	hasArguments = false;

	$(void, event, attachEventListener)(NULL, "module_perform_finished", NULL, &listener_modulesLoaded);

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(NULL, "module_perform_finished", NULL, &listener_modulesLoaded);

	for(GSList *current = clOptions; current != NULL; current = g_slist_next(current)) {
		CLOption *option = (CLOption *) current->data;
		free(option->briefHelp);
		free(option->module);

		if(option->longOpt) {
			free(option->longOpt);
		}

		if(option->shortOpt) {
			free(option->shortOpt);
		}

		free(option);
	}

	g_slist_free(clOptions);

	for(GSList *current = clArguments; current != NULL; current = g_slist_next(current)) {
		CLArgument *argument = (CLArgument *)current->data;
		free(argument->name);
		free(argument->module);
		free(argument->briefHelp);

		free(argument);
	}

	g_slist_free(clArguments);
}

static void listener_modulesLoaded(void *subject, const char *event, void *data, va_list args)
{
	if(!HAS_OPT("h") && !HAS_OPT("help")) {
		return;
	}

	char *execName = $$(char *, getExecutableName)();
	printf("\n%s%s ", "Usage: ", execName);
	free(execName);

	Table *table = $(Table *, plaintext_table, newPlaintextTableFull)(MODULE_TABLE_DEFAULT_ALLOC_ROWS, 3);
	$(int, table, appendTableCol)(table, 3, NULL);

	if(hasOptions && hasArguments) {
		printf("[options] [arguments]\n\n");
		printOptionsHelp(table);
		printArgumentHelp(table, true);
	} else if(hasOptions && !hasArguments) {
		printf("[options]\n\n");
		printOptionsHelp(table);
	} else if (hasArguments && !hasOptions) {
		printf("[arguments]\n\n");
		printArgumentHelp(table, false);
	} else {
		printf("\n\nNo help for usage, options or arguments were given.\n");
	}

	char *output = $(char *, table, getTableString)(table);
	printf("%s", output);

	free(output);
	$(void, table, freeTable)(table);
}

API bool addCLOptionHelp(char *moduleName, char *shortOpt, char *longOpt, char *briefHelp)
{
	// Check params
	if(!moduleName) {
		logWarning("CLI option owner module name must be given to add a new CLI help entry.");
		return false;
	}

	if(!briefHelp) {
		logWarning("Brief help for CLI option help entry must be given.");
		return false;
	}

	if(!(shortOpt || longOpt)) {
		logWarning("Short option or long option must be given to add a new CLI help entry.");
		return false;
	}

	// Create entry
	CLOption *option = ALLOCATE_OBJECT(CLOption);
	option->module = strdup(moduleName);
	option->shortOpt = shortOpt ? strdup(shortOpt) : NULL;
	option->longOpt = longOpt ? strdup(longOpt) : NULL;
	option->briefHelp = strdup(briefHelp);

	clOptions = g_slist_prepend(clOptions, option);

	hasOptions = true;

	return true;
}

API bool addCLArgumentHelp(char *moduleName, char *name, char *briefHelp)
{
	// Check params
	if(!moduleName) {
		logWarning("CLI argument owner module name must be given to add a new CLI help entry.");
		return false;
	}

	if(!name) {
		logWarning("CLI argument name is not given and the new CLI help entry cannot be added.");
		return false;
	}

	if(!briefHelp) {
		logWarning("Brief help for CLI argument help entry must be given.");
		return false;
	}

	// Create entry
	CLArgument *argument = ALLOCATE_OBJECT(CLArgument);
	argument->module = strdup(moduleName);
	argument->name = strdup(name);
	argument->briefHelp = strdup(briefHelp);

	clArguments = g_slist_prepend(clArguments, argument);

	hasArguments = true;
	return true;
}

static void printArgumentHelp(Table *table, bool isAfterOptions)
{
	clArguments = g_slist_reverse(clArguments);

	int headRowIndex = 0;
	if(isAfterOptions) {
		headRowIndex = $(int, table, appendTableRow)(table, 2, NULL) + 1; // first line is a space
	}

	table->table[headRowIndex][0]->content = "Arguments:";

	for(GSList *current = clArguments; current != NULL; current = g_slist_next(current)) {
		CLArgument *argument = (CLArgument *)current->data;
		int rowIndex = $(int, table, appendTableRow)(table, 1, NULL);

		table->table[rowIndex][0]->content = argument->name;
		table->table[rowIndex][1]->content = argument->briefHelp;

		table->table[rowIndex][2]->content = g_strjoin("", "Module: ", argument->module, NULL);
		table->table[rowIndex][2]->freeContent = true;
	}
}

static void printOptionsHelp(Table *table)
{
	clOptions = g_slist_reverse(clOptions);

	table->table[0][0]->content = "Options:";

	for(GSList *current = clOptions; current != NULL; current = g_slist_next(current)) {
		CLOption *option = (CLOption *)current->data;
		int row = $(int, table, appendTableRow)(table, 1, NULL);

		// Add shortOpt & longOpt
		if(option->shortOpt && option->longOpt) {
			table->table[row][0]->content = g_strjoin("", SHORT_OPT_PREFIX, option->shortOpt, ", ", LONG_OPT_PREFIX, option->longOpt, NULL);
			table->table[row][0]->freeContent = true;
		} else if (option->shortOpt && !option->longOpt) {
			table->table[row][0]->content = g_strjoin("", SHORT_OPT_PREFIX, option->shortOpt, NULL);
			table->table[row][0]->freeContent = true;
		} else if (!option->shortOpt && option->longOpt) {
			table->table[row][0]->content = g_strjoin("", LONG_OPT_PREFIX, option->longOpt, NULL);
			table->table[row][0]->freeContent = true;
		}

		// Add brief help
		table->table[row][1]->content = option->briefHelp;

		// Add Module
		table->table[row][2]->content = g_strjoin("", "Module: ", option->module, NULL);
		table->table[row][2]->freeContent = true;
	}
}
