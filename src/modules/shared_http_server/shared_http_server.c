/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *		 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *		 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *			 in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dll.h"
#include "modules/http_server/http_server.h"
#define API
#include "shared_http_server.h"

#define PORT "8888"
#define HOME "[/]+"

MODULE_NAME("shared_http_server");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a shared http server. Other modules should use this library to expose functionality over HTTP (instead of running their own server).");
MODULE_VERSION(0, 0, 2);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("http_server", 0, 1, 2));

/** Struct used to keep track of various stats about the registered modules */
typedef struct {
	/** The name of the module (as registered with prefix */
	char *name;

	/** The number of handlers currently registered. Mainly used for garbage collection */
	int num_handlers;
} ModuleStatus;

/** The actual server being run */
static HttpServer *shared_server;

/** Stores the mapping between module names and the corresponding ModuleStatus structs */
static GHashTable *modules;

static bool homePage(HttpRequest *request, HttpResponse *response, void *userdata);
static ModuleStatus *createModuleStatus(char *name);
static void destroyModuleStatus(ModuleStatus *module_status);

MODULE_INIT
{
	modules = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, (GDestroyNotify) &destroyModuleStatus);
	shared_server = createHttpServer(PORT);
	registerHttpServerRequestHandler(shared_server, HOME, &homePage, NULL);
	if(!startHttpServer(shared_server)) {
		logError("Failed to start shared HTTP server");
		destroyHttpServer(shared_server);
		return false;
	}
	return true;
}

MODULE_FINALIZE
{
	destroyHttpServer(shared_server);
	g_hash_table_destroy(modules);  // Frees/destroys all remaining keys and values.
}

API void registerSharedHttpServerRequestHandlerWithPrefix(char *prefix, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata)
{
	char *path = g_strdup_printf("/%s%s", prefix, hierarchical_regexp);
	registerHttpServerRequestHandler(shared_server, path, handler, userdata);
	free(path);

	ModuleStatus *status = g_hash_table_lookup(modules, prefix);
	if (status == NULL) {
		status = createModuleStatus(prefix);
		g_hash_table_replace(modules, g_strdup(prefix), status);
	}
	status->num_handlers += 1;
	logInfo("Module %s has %d HTTP request handlers remaining in shared HTTP server", prefix, status->num_handlers);
}

API void unregisterSharedHttpServerRequestHandlerWithPrefix(char *prefix, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata)
{
	char *path = g_strdup_printf("/%s%s", prefix, hierarchical_regexp);
	unregisterHttpServerRequestHandler(shared_server, path, handler, userdata);
	free(path);

	ModuleStatus *status = g_hash_table_lookup(modules, prefix);
	if (status == NULL) {
		logWarning("Missing module status for module %s. Skipping stats update", prefix);
	} else {
		status->num_handlers -= 1;
		logInfo("Module %s has %d HTTP request handlers remaining in shared HTTP server", prefix, status->num_handlers);
		if (status->num_handlers <= 0) {
			g_hash_table_remove(modules, prefix); // Key is free'd, value is destroyed.
		}
	}
}

static bool homePage(HttpRequest *request, HttpResponse *response, void *userdata)
{
	appendHttpResponseContent(response, "<h1>Kalisko Webserver</h1>");
	appendHttpResponseContent(response, "<p>Welcome to the Kalisko Webserver. ");

	GString *html_module_list = g_string_new("<ul>");
	GList *module_names = g_hash_table_get_keys(modules);
	int num_modules = 0;
	for (GList *iter = module_names; iter != NULL; iter = iter->next) {
		num_modules += 1;
		g_string_append_printf(html_module_list, "<li><a href=%s>%s</a></li>", (char *)iter->data, (char *)iter->data);
	}
	g_string_append(html_module_list, "</ul>");

	if (num_modules == 0) {
		appendHttpResponseContent(response, "There are no Kalisko modules exposing HTTP functionality.");
	} else {
		appendHttpResponseContent(response, "Modules exposing HTTP functionality:");
		appendHttpResponseContent(response, html_module_list->str);
	}

	g_string_free(html_module_list, true);
	return true;
}

static ModuleStatus *createModuleStatus(char *name)
{
	ModuleStatus *result = ALLOCATE_OBJECT(ModuleStatus);
	result->name = strdup(name);
	result->num_handlers = 0;
	return result;
}

static void destroyModuleStatus(ModuleStatus *module_status)
{
	free(module_status->name);
	free(module_status);
}
