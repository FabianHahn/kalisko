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
#include "hooks.h"
#include "api.h"
#include "lang_php.h"
#include "phpext_kalisko.h"

MODULE_NAME("lang_php");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module provides support for the PHP scripting language");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 2);
MODULE_DEPENDS(MODULE_DEPENDENCY("xcall", 0, 1, 5));

static int ub_write(const char *str, unsigned int str_length TSRMLS_DC);
static void log_message(char *message);
static void sapi_error(int type, const char *fmt, ...);

static char *argvp[2] = {"kalisko", NULL};

MODULE_INIT
{
	php_embed_module.ub_write = &ub_write;
	php_embed_module.log_message = &log_message;
	php_embed_module.sapi_error = &sapi_error;

	if(php_embed_init(1, argvp PTSRMLS_CC) == FAILURE)
	{
		LOG_ERROR("Failed to initialize the PHP embed SAPI!");
	}
	else
	{
		LOG_INFO("Successfully initialized the PHP embed SAPI");
	}

	// Load surgebot php extension
	zend_startup_module(&php_kalisko_ext_entry);

	zend_alter_ini_entry("display_errors", sizeof("display_errors"), "0", sizeof("0") - 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
	zend_alter_ini_entry("log_errors", sizeof("log_errors"), "1", sizeof("1") - 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
	zend_alter_ini_entry("error_log", sizeof("error_log"), "", sizeof("") - 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
	zend_alter_ini_entry("error_reporting", sizeof("error_reporting"), "6143", sizeof("6143") - 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);

	HOOK_ADD(php_out);
	HOOK_ADD(php_log);

	return true;
}

MODULE_FINALIZE
{
	LOG_INFO("Shutting down the PHP SAPI");
	php_embed_shutdown(TSRMLS_C);

	HOOK_DEL(php_out);
	HOOK_DEL(php_log);
}

static int ub_write(const char *str, unsigned int str_length TSRMLS_DC)
{
	if(str_length > 0) {
		HOOK_TRIGGER(php_out, str, str_length);
	}

	return 0;
}

static void log_message(char *message)
{
	HOOK_TRIGGER(php_log, message);
	LOG_WARNING("%s", message);
}

static void sapi_error(int type, const char *fmt, ...)
{
	LOG_ERROR("PHP SAPI error!");
}

API PhpEvalRet evaluatePhp(char *eval)
{
	PhpEvalRet ret;

	zend_first_try
	{
		ret = zend_eval_string(eval, NULL, argvp[0] TSRMLS_CC) == SUCCESS ? EVAL_RET_OK : EVAL_RET_ERROR;
	}
	zend_catch
	{
		ret = EVAL_RET_BAIL;
	}
	zend_end_try();

	return ret;
}
