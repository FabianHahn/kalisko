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
#include <assert.h>
#include <jsapi.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "modules/xcall/xcall.h"

#define API
#include "modules/lang_javascript/lang_javascript.h"
#include "modules/lang_javascript/store.h"
#include "modules/lang_javascript/xcall.h"


MODULE_NAME("lang_javascript");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module provides access to the JavaScript scripting language");
MODULE_VERSION(0, 3, 2);
MODULE_BCVERSION(0, 3, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 5, 3), MODULE_DEPENDENCY("xcall", 0, 2, 3));

static void reportError(JSContext *context, const char *message, JSErrorReport *report);

static JSClass globalClass = {
		"global", JSCLASS_GLOBAL_FLAGS,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSEnvInfo envInfo;
jsval lastReturnValue;

GList *scripts;

MODULE_INIT
{
	JS_CStringsAreUTF8();

	envInfo.runtime = JS_NewRuntime(8L * 1024L * 1024L); // GC after 8MBytes were used
	if(!envInfo.runtime) {
		LOG_ERROR("Could not create JavaScript Runtime.");
		JS_ShutDown();

		return false;
	}

	envInfo.context = JS_NewContext(envInfo.runtime, 8192);
	if(!envInfo.context) {
		LOG_ERROR("Could not create JavaScript Context.");
		JS_DestroyRuntime(envInfo.runtime);
		JS_ShutDown();

		return false;
	}

	JS_SetOptions(envInfo.context, JSOPTION_VAROBJFIX);
	JS_SetVersion(envInfo.context, JSVERSION_LATEST);
	JS_SetErrorReporter(envInfo.context, reportError);

#ifdef JS_GC_ZEAL
	JS_SetGCZeal(envInfo.context, 2);
#endif

	envInfo.globalObject = JS_NewObject(envInfo.context, &globalClass, NULL, NULL);
	if(!envInfo.globalObject) {
		LOG_ERROR("Could not create global JavaScript object");
		JS_DestroyContext(envInfo.context);
		JS_DestroyRuntime(envInfo.runtime);
		JS_ShutDown();

		return false;
	}

	if(!JS_InitStandardClasses(envInfo.context, envInfo.globalObject)) {
		LOG_ERROR("Could not initialize standard classes for global JavaScript object.");
		JS_DestroyContext(envInfo.context);
		JS_DestroyRuntime(envInfo.runtime);
		JS_ShutDown();

		return false;
	}

	jsXCallInit();
	jsAddXCallFunctions(envInfo.context, envInfo.globalObject);
	jsAddStoreFunctions(envInfo.context, envInfo.globalObject);

	return true;
}

MODULE_FINALIZE
{
	for(GList *iter = scripts; iter != NULL; iter = iter->next) {
		JS_RemoveRoot(envInfo.context, &(iter->data));
	}
	g_list_free(scripts);
	scripts = NULL;

	JS_DestroyContext(envInfo.context);
	JS_DestroyRuntime(envInfo.runtime);
	JS_ShutDown();
}

/**
 * Runs the given JavaScript script.
 *
 * @param script	The JavaScript script.
 */
API bool evaluateJavaScript(char *script)
{
	GList *scriptObj = g_list_alloc();

	JSScript *compiledScript = JS_CompileScript(envInfo.context, envInfo.globalObject, script, strlen(script), "_inline", 0);
	if(!compiledScript) {
		LOG_WARNING("Could not compile given JavaScript script.");
		return false;
	}

	scriptObj->data = JS_NewScriptObject(envInfo.context, compiledScript);
	if(!scriptObj) {
		LOG_WARNING("Could not create script object for given JavaScript script.");
		JS_DestroyScript(envInfo.context, compiledScript);

		return false;
	}

	assert(JS_AddNamedRoot(envInfo.context, &(scriptObj->data), "Kalisko JavaScript script"));

	if(!JS_ExecuteScript(envInfo.context, envInfo.globalObject, compiledScript, &lastReturnValue)) {
		LOG_WARNING("Could not execute given JavaScript script.");
		return false;
	}

	scripts = g_list_concat(scripts, scriptObj);

	return true;
}

/**
 * Runs the given JavaScript script file.
 *
 * @param filename	file path to a JavaScript file.
 */
API bool evaluateJavaScriptFile(char *filename)
{
	GList *scriptObj = g_list_alloc();

	JSScript *compiledScript = JS_CompileFile(envInfo.context, envInfo.globalObject, filename);
	if(!compiledScript) {
		LOG_WARNING("Could not compile given JavaScript file: %s.", filename);
		return false;
	}

	scriptObj->data = JS_NewScriptObject(envInfo.context, compiledScript);
	if(!scriptObj) {
		LOG_WARNING("Could not create script object for given JavaScript file: %s", filename);
		JS_DestroyScript(envInfo.context, compiledScript);

		return false;
	}

	assert(JS_AddNamedRoot(envInfo.context, &(scriptObj->data), "Kalisko JavaScript file"));

	if(!JS_ExecuteScript(envInfo.context, envInfo.globalObject, compiledScript, &lastReturnValue)) {
		LOG_WARNING("Could not execute given JavaScript script.");
		return false;
	}

	scripts = g_list_concat(scripts, scriptObj);

	return true;
}

/**
 * Return the last result returned by JS_ExecuteScript.
 *
 * Can be used to get the result after calling evaluateJavaScript or evaluateJavaScriptFile.
 *
 * @return The last result returned by JS_ExecuteScript.
 */
API jsval getJavaScriptLastResult()
{
	return lastReturnValue;
}

/**
 * Returns the globally used JavaScript environment (Runtime, Context, Global Object).
 *
 * This can be used to interact directly with the JavaScript environment.
 *
 * @return The JavaScript environment.
 */
API JSEnvInfo getJavaScriptEnvInfo()
{
	return envInfo;
}

/**
 * Bridge function to put not handled JavaScript errors into the log system.
 *
 * @param context
 * @param message
 * @param report
 */
static void reportError(JSContext *context, const char *message, JSErrorReport *report)
{
	LOG_ERROR("JavaScript error in '%s':'%i': %s", report->filename, (unsigned int)report->lineno, message);
}
