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
#include <string.h>
#include <jsapi.h>

#include "dll.h"
#include "test.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/xcall/xcall.h"
#include "modules/lang_javascript/lang_javascript.h"
#include "modules/lang_javascript/store.h"

#include "api.h"

MODULE_NAME("test_lang_javascript");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the lang_javascript module");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("lang_javascript", 0, 2, 1), MODULE_DEPENDENCY("javascript_core", 0, 1, 0),
		MODULE_DEPENDENCY("xcall", 0, 1, 5), MODULE_DEPENDENCY("store", 0, 5, 3));

static char *testJSScript = "\
function hello(xcall)\
{\
	return \"hello = world, xcall = { function = jsHello }\";\
}\
\
xcall.addFunction(\"jsHello\", hello);";

static char *callHelloJSScript = "xcall.invoke(\"xcall = { function = jsHello }\");";
static char *removeHelloJSScript = "xcall.delFunction(\"jsHello\")";

static void tearUp();

TEST_CASE(callJSFunction);
TEST_CASE(jsCallsXCall);
TEST_CASE(delXCall);

TEST_SUITE_BEGIN(lang_javascript)
	tearUp();
	TEST_CASE_ADD(callJSFunction);
	TEST_CASE_ADD(jsCallsXCall);
	TEST_CASE_ADD(delXCall);
TEST_SUITE_END

static void tearUp()
{
	evaluateJavaScript(testJSScript);
}

TEST_CASE(callJSFunction)
{
	char *call = "xcall = { function = jsHello }";

	GString *retStr = $(GString *, xcall, invokeXCall)(call);
	TEST_ASSERT(retStr != NULL);

	Store *ret = $(Store *, store, parseStoreString)(retStr->str);
	TEST_ASSERT(ret != NULL);
	g_string_free(retStr, true);

	TEST_ASSERT(strcmp($(Store *, store, getStorePath)(ret, "hello")->content.string, "world") == 0);

	$(void, store, freeStore)(ret);

	TEST_PASS;
}

TEST_CASE(jsCallsXCall)
{
	JSEnvInfo envInfo = $(JSEnvInfo, lang_javascript, getJavaScriptEnvInfo)();
	jsval ret;
	char *retStr;

	TEST_ASSERT(JS_EvaluateScript(envInfo.context, envInfo.globalObject, callHelloJSScript, strlen(callHelloJSScript), "test", 0, &ret));
	TEST_ASSERT(JS_ConvertArguments(envInfo.context, 1, &ret, "s", &retStr));

	Store *retStore = $(Store *, store, parseStoreString)(retStr);
	TEST_ASSERT(strcmp($(Store *, store, getStorePath)(retStore, "hello")->content.string, "world") == 0);

	$(void, store, freeStore)(retStore);

	TEST_PASS;
}

TEST_CASE(delXCall)
{
	JSEnvInfo envInfo = $(JSEnvInfo, lang_javascript, getJavaScriptEnvInfo)();
	jsval ret;
	char *retStr;

	TEST_ASSERT(JS_EvaluateScript(envInfo.context, envInfo.globalObject, removeHelloJSScript, strlen(removeHelloJSScript), "test", 0, NULL));

	TEST_ASSERT(JS_EvaluateScript(envInfo.context, envInfo.globalObject, callHelloJSScript, strlen(callHelloJSScript), "test", 0, &ret));
	TEST_ASSERT(JS_ConvertArguments(envInfo.context, 1, &ret, "s", &retStr));

	Store *retStore = $(Store *, store, parseStoreString)(retStr);
	TEST_ASSERT($(Store *, store, getStorePath)(retStore, "xcall/error")->content.string);

	$(void, store, freeStore)(retStore);

	TEST_PASS;
}
