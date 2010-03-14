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
#include <js/jsarray.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "memory_alloc.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"

#include "api.h"
#include "modules/lang_javascript/store.h"

typedef struct {
	JSContext *context;
	JSObject *parentObj;
} JSParseStoreInfo;

static JSBool js_parseStore(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval);
static jsval parseJSStoreValue(Store *store, JSContext *context);
static void parseJSStoreArrayNode(void *key_p, void *value_p, void *state_p);

/**
 * Adds to the given global object functions to work with Store in the JavaScript world.
 *
 * @param context
 * @param globalObj
 */
API void jsAddStoreFunctions(JSContext *context, JSObject *globalObj)
{
	JS_DefineFunction(context, globalObj, "parseStore", &js_parseStore, 1, 0);
}

/**
 * Native implementation of the JavaScript function 'parseStore'. It parses
 * a Store string into a JavaScript object.
 *
 * @param context
 * @param object
 * @param argc
 * @param argv
 * @param rval
 * @return
 */
static JSBool js_parseStore(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval)
{
	char *storeStr;

	if(!JS_ConvertArguments(context, 1, argv, "s", &storeStr)) {
		JS_ReportError(context, "First parameter must be a string.");
		return JS_FALSE;
	}

	Store *store = $(Store *, store, parseStoreString)(storeStr);
	if(!store) {
		JS_ReportError(context, "First parameter is not a string representing a Store.");
		$(void, store, freeStore)(store);
		return JS_FALSE;
	}

	jsval ret = parseJSStoreValue(store, context);
	*rval = ret;

	$(void, store, freeStore)(store);

	return JS_TRUE;
}

/**
 * Converts a single Store value for the JavaScript world.
 *
 * @param store
 * @param context
 * @return
 */
static jsval parseJSStoreValue(Store *store, JSContext *context)
{
	switch(store->type) {
		case STORE_FLOAT_NUMBER:
		{
			jsval val;
			JS_NewDoubleValue(context, store->content.float_number, &val);

			return val;
		}
		case STORE_INTEGER:
		{
			jsval val;
			JS_NewNumberValue(context, store->content.integer, &val);

			return val;
		}
		case STORE_LIST:
		{
			jsval *list = JS_malloc(context, store->content.list->length * sizeof(jsval));

			if(!list) {
				LOG_WARNING("Could not allocate memory for store list.");
				return JSVAL_NULL;
			}

			for(GList *iter = store->content.list->head; iter != NULL; iter = iter->next) {
				*list++ = parseJSStoreValue(iter->data, context);
			}

			return OBJECT_TO_JSVAL(JS_NewArrayObject(context, store->content.list->length, list));
		}
		case STORE_STRING:
		{
			jsval jstr = STRING_TO_JSVAL(JS_NewStringCopyZ(context, store->content.string));
			JS_AddRoot(context, &jstr);

			return jstr;
		}
		case STORE_ARRAY:
		{
			JSObject *obj = JS_NewObject(context, NULL, NULL, NULL);
			JSParseStoreInfo info;

			info.context = context;
			info.parentObj = obj;

			g_hash_table_foreach(store->content.array, &parseJSStoreArrayNode, &info);

			return OBJECT_TO_JSVAL(obj);
		}
		default:
			LOG_WARNING("Unknown Store type. Could not convert it for JavaScript.");
			return JSVAL_NULL;
	}
}

/**
 * Converts a single Array element for the JavaScript world.
 *
 * @param key_p
 * @param value_p
 * @param state_p
 */
static void parseJSStoreArrayNode(void *key_p, void *value_p, void *state_p)
{
	char *key = key_p;
	Store *value = value_p;
	JSParseStoreInfo *info = state_p;

	JS_DefineProperty(info->context, info->parentObj, key, parseJSStoreValue(value, info->context), NULL, NULL, 0);
}
