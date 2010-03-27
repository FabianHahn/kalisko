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

#include <stdio.h>
#include <glib.h>

#undef HAVE_MMAP
#include <sapi/embed/php_embed.h>
#include <TSRM.h>
#include <main/SAPI.h>
#include <zend_ini.h>
#include <php.h>
#include <php_ini.h>

#include "dll.h"
#include "log.h"
#include "module.h"
#include "modules/xcall/xcall.h"
#include "modules/store/store.h"
#include "modules/store/write.h"
#include "api.h"
#include "phpext_kalisko.h"

PHP_MINIT_FUNCTION(kalisko)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(kalisko)
{
	return SUCCESS;
}

/**
 * PHP C function to invoke an XCall
 */
PHP_FUNCTION(invokeXCall)
{
	char *xcall;
	int length;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &xcall, &length) == FAILURE)
	{
		return;
	}

	Store *ret = $(Store *, xcall, invokeXCallByString)(xcall);
	GString *retstr = $(GString *, store, writeStoreGString)(ret);
	ZVAL_STRING(return_value, retstr->str, true);
	g_string_free(retstr, true);
	$(void, store, freeStore)(ret);
}

function_entry php_kalisko_ext_functions[] =
{
	PHP_FE(invokeXCall, NULL)
};

zend_module_entry php_kalisko_ext_entry = {
	STANDARD_MODULE_HEADER,
	"kalisko",
	php_kalisko_ext_functions,
	PHP_MINIT(kalisko),
	PHP_MSHUTDOWN(kalisko),
	NULL,
	NULL,
	NULL,
	"1.0",
	STANDARD_MODULE_PROPERTIES
};
