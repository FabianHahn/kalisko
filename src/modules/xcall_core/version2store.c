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

#include <assert.h>
#include <glib.h>
#include "dll.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#define API

/**
 * Serializes a versions struct into a store
 *
 * @param version		the version struct to convert into a store, must not be NULL
 * @result				the serialized version in store format
 */
API Store *version2Store(Version *version)
{
	assert(version != NULL);

	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "major", $(Store *, store, createStoreIntegerValue)(version->major));
	$(bool, store, setStorePath)(ret, "minor", $(Store *, store, createStoreIntegerValue)(version->minor));
	$(bool, store, setStorePath)(ret, "patch", $(Store *, store, createStoreIntegerValue)(version->patch));
	$(bool, store, setStorePath)(ret, "revision", $(Store *, store, createStoreIntegerValue)(version->revision));

	GString *string = $$(GString *, dumpVersion)(version);
	$(bool, store, setStorePath)(ret, "string", $(Store *, store, createStoreStringValue)(string->str));
	g_string_free(string, true);

	return ret;
}
