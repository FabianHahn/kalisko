/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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

#include <curl/curl.h>
#include <glib.h>

#include "dll.h"

#define API
#include "curl.h"

MODULE_NAME("curl");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("CURL library access");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

size_t writeCurlData(void *buffer, size_t size, size_t nmemb, void *userp);

MODULE_INIT
{
	curl_global_init(CURL_GLOBAL_ALL);

	return true;
}

MODULE_FINALIZE
{
	curl_global_cleanup();
}

/**
 * Requests an URL using the CURL library and returns the results as a string
 *
 * @param url		the url to request
 * @result			the resulting page as a string or NULL on failure
 */
API GString *curlRequestUrl(const char *url)
{
	GString *result = g_string_new("");
	char error[CURL_ERROR_SIZE];

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCurlData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

	LOG_DEBUG("Requesting URL '%s'...", url);

	if(curl_easy_perform(curl) != 0) {
		LOG_ERROR("Failed to read URL '%s': %s", url, error);
		g_string_free(result, true);
		return NULL;
	}

	curl_easy_cleanup(curl);

	return result;
}

size_t writeCurlData(void *buffer, size_t size, size_t nmemb, void *userp)
{
	GString *result = (GString *) userp;

	g_string_append_len(result, buffer, size * nmemb);

	return size * nmemb;
}

