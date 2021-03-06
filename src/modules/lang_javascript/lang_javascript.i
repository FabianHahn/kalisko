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


#ifndef LANG_JAVASCRIPT_LANG_JAVASCRIPT_H
#define LANG_JAVASCRIPT_LANG_JAVASCRIPT_H

#include <jsapi.h>

typedef struct {
	JSContext *context;
	JSRuntime *runtime;
	JSObject *globalObject;
} JSEnvInfo;


/**
 * Runs the given JavaScript script.
 *
 * @param script	The JavaScript script.
 */
API bool evaluateJavaScript(char *script);

/**
 * Runs the given JavaScript script file.
 *
 * @param filename	file path to a JavaScript file.
 */
API bool evaluateJavaScriptFile(char *filename);

/**
 * Return the last result returned by JS_ExecuteScript.
 *
 * Can be used to get the result after calling evaluateJavaScript or evaluateJavaScriptFile.
 *
 * @return The last result returned by JS_ExecuteScript.
 */
API jsval getJavaScriptLastResult();

/**
 * Returns the globally used JavaScript environment (Runtime, Context, Global Object).
 *
 * This can be used to interact directly with the JavaScript environment.
 *
 * @return The JavaScript environment.
 */
API JSEnvInfo getJavaScriptEnvInfo();

#endif
