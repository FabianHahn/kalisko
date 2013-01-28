/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
 * Copyright (c) 2012, Google Inc.
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
#include "modules/curl/curl.h"
#include "modules/xml/xml.h"
#include "modules/http_server/http_server.h"
#define API
#include "feed.h"

MODULE_NAME("feed");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to track XML feeds");
MODULE_VERSION(0, 3, 2);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xml", 0, 1, 2), MODULE_DEPENDENCY("curl", 0, 1, 1), MODULE_DEPENDENCY("http_server", 0, 1, 2));

#define FEED_LIMIT 200
#define GENERIC_FEED_URI "^/feeds/%s.*$"

TIMER_CALLBACK(feed_update);
static bool indexHandler(HttpRequest *request, HttpResponse *response, void *userdata_p);
static bool feedHandler(HttpRequest *request, HttpResponse *response, void *userdata_p);
static bool compareFeedContentEntries(GHashTable *first, GHashTable *second);
static void freeFeed(void *feed_p);

/** GHashTable associating feed names with Feed objects */
static GHashTable *feeds;

/** HTTP Server to serve feeds */
static HttpServer *http;

MODULE_INIT
{
	http = createHttpServer("1337");
	registerHttpServerRequestHandler(http, "^/[^/]*$", &indexHandler, NULL);

	if(!startHttpServer(http)) {
		logError("Failed to start feed HTTP server");
		destroyHttpServer(http);
		return false;
	}

	feeds = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeFeed);
	TIMER_ADD_TIMEOUT(0, feed_update);

	createFeed("generations", "http://generations.fr/winradio/prog.xml");
	addFeedField("generations", "time", "/prog/morceau[@id='1']/date_prog");
	addFeedField("generations", "artist", "/prog/morceau[@id='1']/chanteur");
	addFeedField("generations", "title", "/prog/morceau[@id='1']/chanson");
	enableFeed("generations");

	return true;
}

MODULE_FINALIZE
{
	g_hash_table_destroy(feeds);
}

TIMER_CALLBACK(feed_update)
{
	GHashTableIter iter;
	char *name;
	Feed *feed;
	g_hash_table_iter_init(&iter, feeds);
	while(g_hash_table_iter_next(&iter, (void **) &name, (void **) &feed)) {
		if(!feed->enabled) {
			continue;
		}

		GString *xml;
		if((xml = curlRequestUrl(feed->url)) == NULL) {
			continue;
		}

		xmlDocPtr document;
		if((document = parseXmlString(xml->str)) == NULL) {
			g_string_free(xml, true);
			continue;
		}

		GHashTable *entry = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &free);

		for(GList *iter = feed->fields->head; iter != NULL; iter = iter->next) {
			FeedField *field = iter->data;
			GString *value = evaluateXPathExpressionFirst(document, field->expression);
			if(value != NULL) {
				logInfo("Feed '%s' field '%s' value: %s", name, field->name, value->str);
				g_hash_table_insert(entry, strdup(field->name), value->str);
				g_string_free(value, false);
			}
		}

		xmlFreeDoc(document);

		if(!g_queue_is_empty(feed->content)) {
			// Check if the new content element is different from the last recorded one
			GHashTable *last = g_queue_peek_head(feed->content);
			if(compareFeedContentEntries(last, entry)) {
				g_hash_table_destroy(entry);
				logInfo("Feed entry for feed '%s' already exists, skipping", feed->name);
				continue;
			}
		}

		g_queue_push_head(feed->content, entry);
		logInfo("Added new feed content entry for '%s'", feed->name);

		if(g_queue_get_length(feed->content) > FEED_LIMIT) {
			GHashTable *first = g_queue_pop_tail(feed->content);
			g_hash_table_destroy(first);
		}
	}

	TIMER_ADD_TIMEOUT(60 * G_USEC_PER_SEC, feed_update);
}

API bool createFeed(const char *name, const char *url)
{
	if(g_hash_table_lookup(feeds, name) != NULL) {
		logError("Failed to create feed '%s': A feed with that name already exists", name);
		return false;
	}

	Feed *feed = ALLOCATE_OBJECT(Feed);
	feed->name = strdup(name);
	feed->url = strdup(url);
	feed->fields = g_queue_new();
	feed->content = g_queue_new();
	feed->enabled = false;

	g_hash_table_insert(feeds, strdup(name), feed);

	GString *regex = g_string_new("");
	g_string_append_printf(regex, GENERIC_FEED_URI, name);
	registerHttpServerRequestHandler(http, regex->str, &feedHandler, feed);
	g_string_free(regex, true);

	return true;
}

API bool addFeedField(const char *name, const char *fieldName, const char *expression)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		logError("Failed to add field to feed '%s': A feed with that name doesn't exist", name);
		return false;
	}

	if(feed->enabled) {
		logError("Failed to add field to feed '%s': Feed is already enabled", name);
		return false;
	}

	for(GList *iter = feed->fields->head; iter != NULL; iter = iter->next) {
		FeedField *field = iter->data;

		if(g_strcmp0(field->name, fieldName) == 0) {
			logError("Failed to add field '%s' to feed '%s': A field with that name already exists", name, fieldName);
			return false;
		}
	}

	FeedField *field = ALLOCATE_OBJECT(FeedField);
	field->name = strdup(fieldName);
	field->expression = strdup(expression);
	g_queue_push_tail(feed->fields, field);

	return true;
}

API bool deleteFeedField(const char *name, const char *fieldName)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		logError("Failed to add field to feed '%s': A feed with that name doesn't exist", name);
		return false;
	}

	if(feed->enabled) {
		logError("Failed to add field to feed '%s': Feed is already enabled", name);
		return false;
	}

	for(GList *iter = feed->fields->head; iter != NULL; iter = iter->next) {
		FeedField *field = iter->data;

		if(g_strcmp0(field->name, fieldName) == 0) {
			free(field->name);
			free(field->expression);
			free(field);
			g_queue_delete_link(feed->fields, iter);
			return true;
		}
	}

	logError("Failed to delete field '%s' from feed '%s': A field with that name doesn't exist", fieldName, name);
	return false;
}

API bool enableFeed(const char *name)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		logError("Failed to enable feed '%s': A feed with that name doesn't exist", name);
		return false;
	}

	if(feed->enabled) {
		logError("Failed to enable feed '%s': Feed is already enabled", name);
		return false;
	}

	feed->enabled = true;
	return true;
}

API Feed *getFeed(const char *name)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		logError("Failed to get feed '%s': A feed with that name doesn't exist", name);
		return NULL;
	}

	return feed;
}

API bool deleteFeed(const char *name)
{
	return g_hash_table_remove(feeds, name);
}

static bool indexHandler(HttpRequest *request, HttpResponse *response, void *userdata_p)
{
	appendHttpResponseContent(response, "Currently tracked feeds:<br/><br/>");

	GHashTableIter iter;
	char *name;
	Feed *feed;
	g_hash_table_iter_init(&iter, feeds);
	while(g_hash_table_iter_next(&iter, (void **) &name, (void **) &feed)) {
		appendHttpResponseContent(response, "<a href=\"feeds/%s\">%s</a><br />", name, name);
	}

	return true;
}

static bool feedHandler(HttpRequest *request, HttpResponse *response, void *userdata_p)
{
	Feed *feed = userdata_p;

	appendHttpResponseContent(response, "<html>\n<head>\n<style>table {\nborder-collapse:collapse;\n}\ntd {\nborder: 1px solid black;\npadding: 2px 5px;\n}\nth {\nborder: 1px solid black;\npadding: 2px 5px;\n}\n</style>\n</head>\n<body>\n<h3>Feed '%s'</h3>\n<table>\n<tr>\n", feed->name);

	for(GList *iter = feed->fields->head; iter != NULL; iter = iter->next) {
		FeedField *field = iter->data;
		appendHttpResponseContent(response, "<th>%s</th>", field->name);
	}

	appendHttpResponseContent(response, "\n</tr>\n");

	for(GList *iter = feed->content->head; iter != NULL; iter = iter->next) {
		GHashTable *entry = iter->data;

		appendHttpResponseContent(response, "<tr>\n");

		for(GList *iter = feed->fields->head; iter != NULL; iter = iter->next) {
			FeedField *field = iter->data;
			appendHttpResponseContent(response, "<td>%s</td>", g_hash_table_lookup(entry, field->name));
		}

		appendHttpResponseContent(response, "</tr>\n");
	}

	appendHttpResponseContent(response, "</table>\n</body>\n</html>");

	return true;
}

/**
 * Compares two feed content entries
 *
 * @param first			the first entry to compare
 * @param second		the second entry to compare
 * @param 				true if the entries are equal, false otherwise
 */
static bool compareFeedContentEntries(GHashTable *first, GHashTable *second)
{
	GHashTableIter iter;
	char *field;
	char *value;
	g_hash_table_iter_init(&iter, first);
	while(g_hash_table_iter_next(&iter, (void **) &field, (void **) &value)) {
		char *value2 = g_hash_table_lookup(second, field);
		if(g_strcmp0(value, value2) != 0) {
			return false;
		}
	}

	return true;
}

static void freeFeed(void *feed_p)
{
	Feed *feed = feed_p;

	GString *regex = g_string_new("");
	g_string_append_printf(regex, GENERIC_FEED_URI, feed->name);
	unregisterHttpServerRequestHandler(http, regex->str, &feedHandler, feed);
	g_string_free(regex, true);

	free(feed->name);
	free(feed->url);

	for(GList *iter = feed->fields->head; iter != NULL; iter = iter->next) {
		FeedField *field = iter->data;
		free(field->name);
		free(field->expression);
		free(field);
	}
	g_queue_free(feed->fields);

	for(GList *iter = feed->content->head; iter != NULL; iter = iter->next) {
		g_hash_table_destroy(iter->data);
	}
	g_queue_free(feed->content);

	free(feed);
}
