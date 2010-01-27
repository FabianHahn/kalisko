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
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "module.h"
#include "modules/xcall/xcall.h"
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/path.h"
#include "modules/store/write.h"
#include "api.h"


MODULE_NAME("xcall_core");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module which offers an XCall API to the Kalisko Core");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 1, 5), MODULE_DEPENDENCY("store", 0, 6, 0));

static GString *xcall_requestModule(const char *xcall);
static GString *xcall_revokeModule(const char *xcall);

MODULE_INIT
{
	$(bool, xcall, addXCallFunction)("requestModule", &xcall_requestModule);
	$(bool, xcall, addXCallFunction)("revokeModule", &xcall_revokeModule);

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("requestModule");
	$(bool, xcall, delXCallFunction)("revokeModule");
}

/**
 * XCallFunction to request a module
 * XCall parameters:
 *  * string module		the module to request
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall in serialized store form
 * @result			a return value in serialized store form
 */
static GString *xcall_requestModule(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcs, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, requestModule)(modname);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	GString *ret = $(GString *, store, writeStoreGString)(retstore);
	$(void, store, freeStore)(xcs);
	$(void, store, freeStore)(retstore);

	return ret;
}

/**
 * XCallFunction to revoke a module
 * XCall parameters:
 *  * string module		the module to revoke
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall in serialized store form
 * @result			a return value in serialized store form
 */
static GString *xcall_revokeModule(const char *xcall)
{
	Store *xcs = $(Store *, store, parseStoreString)(xcall);
	Store *retstore = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(retstore, "xcall", $(Store *, store, createStoreArrayValue)(NULL));

	Store *module = $(Store *, store, getStorePath)(xcs, "module");

	if(module == NULL || module->type != STORE_STRING) {
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(0));
		$(bool, store, setStorePath)(retstore, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'module'"));
	} else {
		char *modname = module->content.string;
		bool loaded = $$(bool, revokeModule)(modname);
		$(bool, store, setStorePath)(retstore, "success", $(Store *, store, createStoreIntegerValue)(loaded));
	}

	GString *ret = $(GString *, store, writeStoreGString)(retstore);
	$(void, store, freeStore)(xcs);
	$(void, store, freeStore)(retstore);

	return ret;
}
