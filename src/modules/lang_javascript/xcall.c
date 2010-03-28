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
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"
#include "modules/store/parse.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "modules/xcall/xcall.h"

#include "api.h"
#include "modules/lang_javascript/lang_javascript.h"
#include "modules/lang_javascript/store.h"
#include "modules/lang_javascript/xcall.h"

static JSBool js_invokeXCall(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval);
static JSBool js_addXCallFunction(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval);
static JSBool js_delXCallFunction(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval);

static Store *jsInvokeXCallFunction(Store *xcall);

typedef struct {
	JSContext *context;
	JSObject *object;
	jsval *function;
} JSFunctionInfo;

JSClass xcallClass = {
		"xcall",
		JSCLASS_GLOBAL_FLAGS,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
		JSCLASS_NO_OPTIONAL_MEMBERS
};

/**
 * The key is the name of a registered JavaScript function and the value
 * is a reference to a JSFunctionInfo.
 */
static GHashTable *functions;

/**
 * Initialize needed variables, structures and so on.
 */
API void jsXCallInit()
{
	functions = g_hash_table_new(&g_str_hash, &g_str_equal);
}

/**
 * Modifies given context and global object to be able to use XCall in JavaScript.
 *
 * @param context
 * @param globalObject
 */
API void jsAddXCallFunctions(JSContext *context, JSObject *globalObject)
{
	JSObject *xcallObj = JS_DefineObject(context, globalObject, "xcall", &xcallClass, NULL, 0);
	JS_DefineFunction(context, xcallObj, "invoke", &js_invokeXCall, 1, 0);
	JS_DefineFunction(context, xcallObj, "addFunction", &js_addXCallFunction, 2, 0);
	JS_DefineFunction(context, xcallObj, "delFunction", &js_delXCallFunction, 1, 0);
}

/**
 * Native function representing the 'xcall.invokeXCall' function in JavaScript.
 * The function takes a string or an object representing a XCall.
 *
 * @param context
 * @param object
 * @param argc
 * @param argv
 * @param rval
 * @return
 */
static JSBool js_invokeXCall(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval)
{
	// convert the given parameter
	char *xcall;
	Store *ret;
	if(JSVAL_IS_OBJECT(argv[0])) {
		Store *convStore = $(Store *, lang_javascript, javaScriptValueToStore)(argv[0], context);

		if(!convStore) {
			JS_ReportError(context, "Given parameter must be a object representing a XCall.");
			return JS_FALSE;
		}

		ret = $(Store *, xcall, invokeXCall)(convStore);

		$(void, store, freeStore)(convStore);
	} else if(JS_ConvertArguments(context, 1, argv, "s", &xcall)) {
		ret = $(Store *, xcall, invokeXCallByString)(xcall);
	} else {
		JS_ReportError(context, "Given parameter must be a string or object representing a XCall.");
		return JS_FALSE;
	}

	if(!JS_EnterLocalRootScope(context)) {
		JS_ReportError(context, "Could not enter Local Root Scope.");
		return JS_FALSE;
	}

	// return the result as JavaScript store object
	*rval = storeToJavaScriptValue(ret, context);

	JS_LeaveLocalRootScope(context);

	$(void, store, freeStore)(ret);

	return JS_TRUE;

}

/**
 * Native function representing the 'xcall.addXCallFunction' in JavaScript.
 *
 * @param context
 * @param object
 * @param argc
 * @param argv
 * @param rval
 * @return
 */
static JSBool js_addXCallFunction(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval)
{
	// convert the given parameters
	char *functionName;
	JSObject *functionObj;
	if(!JS_ConvertArguments(context, 2, argv, "so", &functionName, &functionObj)) {
		JS_ReportError(context, "Given parameters are not valid. First parameter must be a string (function name) and the second one the function itself.");
		return JS_FALSE;
	}

	if(!JS_ObjectIsFunction(context, functionObj)) {
		JS_ReportError(context, "Second parameter must be a function.");
		return JS_FALSE;
	}

	// add the function to the known JavaScript functions
	JSFunctionInfo *info = ALLOCATE_OBJECT(JSFunctionInfo);
	info->context = context;
	info->object = object;
	info->function = JS_malloc(context, sizeof(jsval));

	JS_ConvertValue(context, argv[1], JSTYPE_FUNCTION, info->function);

	g_hash_table_insert(functions, strdup(functionName), info);

	// register the function to XCall
	if(!$(bool, xcall, addXCallFunction)(functionName, &jsInvokeXCallFunction)) {
		JS_ReportError(context, "Could not add function to XCall.");
		free(info);

		return JS_FALSE;
	}

	LOG_ERROR("Added JavaScript function as XCall function: %s", functionName);

	return JS_TRUE;
}

/**
 * Native function representing the 'xcall.delXCallFunction' in JavaScript.
 *
 * @param context
 * @param object
 * @param argc
 * @param argv
 * @param rval
 * @return
 */
static JSBool js_delXCallFunction(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval)
{
	// convert the given parameters
	char *functionName;
	if(!JS_ConvertArguments(context, 1, argv, "s", &functionName)) {
		JS_ReportError(context, "Given parameter must be a string.");
		return JS_FALSE;
	}

	// remove the function
	if(!g_hash_table_remove(functions, functionName)) {
		JS_ReportError(context, "The function '%s' could not be removed from the internal Hash Table.", functionName);
		return JS_FALSE;
	}

	if(!$(bool, xcall, delXCallFunction)(functionName)) {
		JS_ReportError(context, "The function '%s' could not be removed from XCall.", functionName);
		return JS_FALSE;
	}

	LOG_ERROR("Removed JavaScript function as XCall function: %s", functionName);
	return JS_TRUE;
}

/**
 * Bridge between the XCall invoke an the registered function in the JavaScript world.
 *
 * @param xcall
 * @return
 */
static Store *jsInvokeXCallFunction(Store *xcall)
{
	Store *functionName = $(Store *, store, getStorePath)(xcall, "xcall/function");
	assert(functionName->type == STORE_STRING);

	JSFunctionInfo *function = g_hash_table_lookup(functions, functionName->content.string);
	if(!function) {
		GString *errorStr = g_string_new("");
		g_string_append_printf(errorStr, "Could not find JavaScript function '%s'.", functionName->content.string);

		Store *ret = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(errorStr->str));

		g_string_free(errorStr, true);

		return ret;
	}

	jsval retValue;
	jsval argv[1];

	if(!JS_EnterLocalRootScope(function->context)) {
		char *errStr = "Could not enter local root scope. XCall Invoke stopped";
		LOG_WARNING("%s", errStr);

		Store *ret = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(errStr));

		JS_LeaveLocalRootScope(function->context);

		return ret;
	}

	argv[0] = storeToJavaScriptValue(xcall, function->context);

	if(!JS_CallFunctionValue(function->context, function->object, *function->function, 1, argv, &retValue)) {
		GString *errorStr = g_string_new("");
		g_string_append_printf(errorStr, "Failed calling JavaScript function '%s'.", functionName->content.string);

		Store *ret = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(errorStr->str));

		g_string_free(errorStr, true);

		JS_LeaveLocalRootScope(function->context);

		return ret;
	}

	if(!JSVAL_IS_OBJECT(retValue)) {
		GString *errorStr = g_string_new("");

		g_string_append_printf(errorStr, "JavaScript function '%s' did not return an object.", functionName->content.string);

		Store *ret = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(errorStr->str));

		g_string_free(errorStr, true);

		JS_LeaveLocalRootScope(function->context);

		return ret;
	}

	Store *retStore = javaScriptValueToStore(retValue, function->context);

	if(!retStore) {
		GString *errorStr = g_string_new("");

		g_string_append_printf(errorStr, "JavaScript function '%s' did not return a parseable Store JavaScript object.", functionName->content.string);

		Store *ret = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStoreArrayValue)(NULL));
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)(errorStr->str));

		g_string_free(errorStr, true);

		JS_LeaveLocalRootScope(function->context);

		return ret;
	}

	JS_LeaveLocalRootScope(function->context);

	return retStore;
}
