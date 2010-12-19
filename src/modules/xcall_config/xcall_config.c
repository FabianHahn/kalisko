/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/xcall/xcall.h"
#include "api.h"

MODULE_NAME("xcall_config");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall module for config");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("store", 0, 6, 4), MODULE_DEPENDENCY("xcall", 0, 2, 6));

static Store *xcall_reloadConfig(Store *xcall);

MODULE_INIT
{
	if(!$(bool, xcall, addXCallFunction)("reloadConfig", xcall_reloadConfig)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("reloadConfig");
}

/**
 * XCallFunction to execute the reloadConfig.
 *
 * On success the returned Store contains just an integer 'success' = 1 else
 * 'success' = 0.
 *
 * @param xcall the xcall as Store
 * @return a return value as Store
 */
static Store *xcall_reloadConfig(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();

	$(void, config, reloadConfig)();

	$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)(1));

	return ret;
}
