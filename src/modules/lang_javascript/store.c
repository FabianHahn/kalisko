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
#include "memory_alloc.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"

#define API
#include "modules/lang_javascript/store.h"
#include "modules/lang_javascript/xcall.h"

static JSBool js_parseStore(JSContext *context, JSObject *object, uintN argc, jsval *argv, jsval *rval);

API void jsAddStoreFunctions(JSContext *context, JSObject *globalObj)
{
	JS_DefineFunction(context, globalObj, "parseStore", &js_parseStore, 1, 0);
}

/**
 * Native implementation of the JavaScript function 'parseStore'. It parses
 * a Store string into a JavaScript object/value.
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

	// Entering a local root scope: All created JavaScript things are not GCed
	if(!JS_EnterLocalRootScope(context)) {
		JS_ReportError(context, "Could not create Local Root Scope.");
		return JS_FALSE;
	}

	jsval ret = storeToJavaScriptValue(store, context);
	*rval = ret;

	// After we set the return value and referenced our new object we can let GC run again
	JS_LeaveLocalRootScope(context);
	JS_MaybeGC(context);

	$(void, store, freeStore)(store);

	return JS_TRUE;
}

API jsval storeToJavaScriptValue(Store *store, JSContext *context)
{
	assert(store != NULL);
	switch(store->type) {
		case STORE_ARRAY:
		{
			GHashTableIter iter;
			void *key;
			void *value;

			g_hash_table_iter_init(&iter, store->content.array);

			JSObject *obj = JS_NewObject(context, NULL, NULL, NULL);
			if(!obj) {
				JS_ReportError(context, "Could not create new object for Store Array");
				return JSVAL_NULL;
			}

			while(g_hash_table_iter_next(&iter, &key, &value)) {
				char *keyStr = key;
				Store *valueStore = value;

				assert(JS_DefineProperty(context, obj, keyStr, storeToJavaScriptValue(valueStore, context), NULL, NULL, JSPROP_ENUMERATE));
			}

			return OBJECT_TO_JSVAL(obj);
		}
		case STORE_LIST:
		{
			jsval *list = JS_malloc(context, store->content.list->length * sizeof(jsval));

			if(!list) {
				logWarning("Could not allocate memory for store list.");
				return JSVAL_NULL;
			}

			for(GList *iter = store->content.list->head; iter != NULL; iter = iter->next) {
				*list++ = storeToJavaScriptValue(iter->data, context);
			}

			assert(JS_NewArrayObject(context, store->content.list->length, list));
			return *list;
		}
		case STORE_FLOAT_NUMBER:
		{
			jsval val;
			assert(JS_NewNumberValue(context, store->content.float_number, &val));

			return val;
		}
		case STORE_INTEGER:
		{
			jsval val;
			assert(JS_NewNumberValue(context, store->content.integer, &val));

			return val;
		}
		case STORE_STRING:
		{
			JSString *str = JS_NewStringCopyZ(context, store->content.string);
			assert(str);

			return STRING_TO_JSVAL(str);
		}
		default:
			logError("Unknown Store Type. This is a bug, please report it");
			return JSVAL_NULL;
	}
}

API Store *javaScriptValueToStore(jsval value, JSContext *context)
{
	switch(JS_TypeOfValue(context, value)) {
		case JSTYPE_OBJECT:
		{
			JSObject *obj = JSVAL_TO_OBJECT(value);
			if(JS_IsArrayObject(context, obj)) {
				GQueue *list = g_queue_new();
				unsigned int length;

				JS_GetArrayLength(context, obj, &length);

				for(unsigned int i = 0; i < length; i++) {
					jsval ret;
					assert(JS_GetElement(context, obj, i, &ret));
					g_queue_push_tail(list, javaScriptValueToStore(ret, context));
				}

				Store *store = $(Store *, store, createStoreListValue)(list);

				return store;
			} else {
				GHashTable *array = g_hash_table_new(&g_str_hash, &g_str_equal);

				JSIdArray *arrIDs = JS_Enumerate(context, obj);
				for(int i = 0; i < arrIDs->length; i++) {
					jsval propVal;
					jsval propName;

					assert(JS_IdToValue(context, arrIDs->vector[i], &propName));
					assert(JS_GetPropertyById(context, obj, arrIDs->vector[i], &propVal));

					g_hash_table_insert(array, JS_GetStringBytes(JS_ValueToString(context, propName)),
							javaScriptValueToStore(propVal, context));
				}

				JS_DestroyIdArray(context, arrIDs);

				Store *store = $(Store *, store, createStoreArrayValue)(array);

				return store;
			}
		}
		case JSTYPE_STRING:
			return $(Store *, store, createStoreStringValue)(JS_GetStringBytes(JSVAL_TO_STRING(value)));
		case JSTYPE_NUMBER:
			if(JSVAL_IS_INT(value)) {
				int convValue;
				assert(JS_ValueToInt32(context, value, &convValue));

				return $(Store *, store, createStoreIntegerValue)(convValue);
			} else if(JSVAL_IS_DOUBLE(value)) {
				double convValue;
				assert(JS_ValueToNumber(context, value, &convValue));

				return $(Store *, store, createStoreFloatNumberValue)(convValue);
			}
		case JSTYPE_BOOLEAN:
			return $(Store *, store, createStoreIntegerValue)(JSVAL_IS_BOOLEAN(value) ? 1 : 0);
		default:
			logWarning("Could not convert JavaScript Type for Store.");
			return $(Store *, store, createStore)();
	}
}
