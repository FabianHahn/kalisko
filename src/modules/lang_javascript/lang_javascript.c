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
#include <js/jsapi.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"

#include "api.h"


MODULE_NAME("lang_javascript");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

static void reportError(JSContext *context, const char *message, JSErrorReport *report);
static JSBool js_logError(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval);

static JSClass globalClass = {
		"global", JSCLASS_GLOBAL_FLAGS,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSRuntime *runtime;
static JSContext *context;
static JSObject *globalObject;


MODULE_INIT
{
	runtime = JS_NewRuntime(8L * 1024L * 1024L);
	if(!runtime) {
		LOG_ERROR("Could not create JavaScript Runtime.");
		return false;
	}

	context = JS_NewContext(runtime, 8192);
	if(!context) {
		LOG_ERROR("Could not create JavaScript Context.");
		JS_DestroyRuntime(runtime);

		return false;
	}

	JS_SetOptions(context, JSOPTION_VAROBJFIX);
	JS_SetVersion(context, JSVERSION_DEFAULT);
	JS_SetErrorReporter(context, reportError);

	globalObject = JS_NewObject(context, &globalClass, NULL, NULL);
	if(!globalObject) {
		LOG_ERROR("Could not create global JavaScript object");
		JS_DestroyContext(context);
		JS_DestroyRuntime(runtime);

		return false;
	}


	JS_DefineFunction(context, globalObject, "logError", js_logError, 1, 0);
	jsval value;
	char *testScript = "logError('Ich bin ein JavaScript, das in Kalisko ausgeführt wurde!!!');";
	JS_EvaluateScript(context, globalObject, testScript, strlen(testScript), "inline", 0, &value);

	return true;
}

MODULE_FINALIZE
{
	JS_DestroyContext(context);
	JS_DestroyRuntime(runtime);
	JS_ShutDown();
}

static void reportError(JSContext *context, const char *message, JSErrorReport *report)
{
	LOG_ERROR("Following error occured in JavaScript Engine: %s", message);
}

static JSBool js_logError(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval)
{
	const char *str;
	JS_ConvertArguments(context, argc, argv, "s", &str);

	LOG_ERROR("----------> %s", str);

	return JS_TRUE;
}
