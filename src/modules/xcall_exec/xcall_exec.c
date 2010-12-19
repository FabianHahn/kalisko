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

#include "dll.h"
#include "types.h"
#include "memory_alloc.h"
#include "util.h"
#include "log.h"
#include "modules/exec/exec.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/xcall/xcall.h"
#include "api.h"

static Store *xcall_executeShellCommand(Store *xcall);
static Store *xcall_executeShellCommandArgs(Store *xcall);

MODULE_NAME("xcall_exec");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall module for exec");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("exec", 0, 1, 0), MODULE_DEPENDENCY("store", 0, 6, 4), MODULE_DEPENDENCY("xcall", 0, 2, 6));

MODULE_INIT
{
	if(!$(bool, xcall, addXCallFunction)("executeShellCommand", &xcall_executeShellCommand)) {
		return false;
	}

	if(!$(bool, xcall, addXCallFunction)("executeShellCommandArgs", &xcall_executeShellCommandArgs)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("executeShellCommand");
	$(bool, xcall, delXCallFunction)("executeShellCommandArgs");
}

/**
 * XCallFunction to execute a shell command
 * XCall parameters:
 *  * string command	the shell command to execute
 * XCall result:
 * 	* string output		the output of the executed command
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_executeShellCommand(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	Store *command;

	if((command = $(Store *, store, getStorePath)(xcall, "command")) == NULL || command->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'command'"));
		return ret;
	}

	GString *output = $(GString *, exec, executeShellCommand)(command->content.string);
	$(bool, store, setStorePath)(ret, "output", $(Store *, store, createStoreStringValue)(output->str));

	return ret;
}

/**
 * XCallFunction to execute a shell command by a list of arguments
 * XCall parameters:
 *  * list args 		a string list of arguments
 * XCall result:
 * 	* string output		the output of the executed command
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_executeShellCommandArgs(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	Store *argsp;

	if((argsp = $(Store *, store, getStorePath)(xcall, "args")) == NULL || argsp->type != STORE_LIST) {
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory list parameter 'args'"));
		return ret;
	}

	int length = g_queue_get_length(argsp->content.list);
	char **args = ALLOCATE_OBJECTS(char *, length + 1);
	memset(args, 0, sizeof(char *) * (length + 1));

	int i = 0;

	for(GList *iter = argsp->content.list->head; iter != NULL; iter = iter->next, i++) {
		Store *entry = iter->data;

		if(entry->type != STORE_STRING) {
			g_strfreev(args);
			GString *error = g_string_new("");
			g_string_append_printf(error, "args list parameter %d is not of type string", i);
			$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());
			$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(error->str));
			g_string_free(error, true);
			return ret;
		}

		args[i] = strdup(entry->content.string);
	}

	GString *output = $(GString *, exec, executeShellCommandArgs)(args);
	$(bool, store, setStorePath)(ret, "output", $(Store *, store, createStoreStringValue)(output->str));

	g_strfreev(args);

	return ret;
}
