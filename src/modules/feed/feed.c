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


#include <glib.h>
#include "dll.h"
#include "modules/curl/curl.h"
#include "modules/xml/xml.h"
#define API
#include "feed.h"

MODULE_NAME("feed");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to track XML feeds");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 2, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xml", 0, 1, 2), MODULE_DEPENDENCY("curl", 0, 1, 1));

TIMER_CALLBACK(feed_update);
static bool compareFeedContentEntries(GHashTable *first, GHashTable *second);
static void freeFeed(void *feed_p);

/** GHashTable associating feed names with Feed objects */
GHashTable *feeds;

MODULE_INIT
{
	feeds = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeFeed);
	TIMER_ADD_TIMEOUT(0, feed_update);

	createFeed("generations", "http://generations.fr/winradio/prog.xml");
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

		GHashTableIter iter2;
		char *field;
		char *expression;
		g_hash_table_iter_init(&iter2, feed->fields);
		while(g_hash_table_iter_next(&iter2, (void **) &field, (void **) &expression)) {
			GString *value = evaluateXPathExpressionFirst(document, expression);
			if(value != NULL) {
				LOG_DEBUG("Feed '%s' field '%s' value: %s", name, field, value->str);
				g_hash_table_insert(entry, strdup(field), value->str);
				g_string_free(value, false);
			}
		}

		xmlFreeDoc(document);

		if(!g_queue_is_empty(feed->content)) {
			// Check if the new content element is different from the last recorded one
			GHashTable *last = g_queue_peek_tail(feed->content);
			if(compareFeedContentEntries(last, entry)) {
				g_hash_table_destroy(entry);
				LOG_DEBUG("Feed entry for feed '%s' already exists, skipping", feed->name);
				continue;
			}
		}

		g_queue_push_tail(feed->content, entry);
		LOG_DEBUG("Added new feed content entry for '%s'", feed->name);
	}

	TIMER_ADD_TIMEOUT(10 * G_USEC_PER_SEC, feed_update);
}

/**
 * Creates a feed
 *
 * @param name			the name of the feed
 * @param url			the URL of the feed
 * @result				true if successful
 */
API bool createFeed(const char *name, const char *url)
{
	if(g_hash_table_lookup(feeds, name) != NULL) {
		LOG_ERROR("Failed to create feed '%s': A feed with that name already exists", name);
		return false;
	}

	Feed *feed = ALLOCATE_OBJECT(Feed);
	feed->name = strdup(name);
	feed->url = strdup(url);
	feed->fields = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &free);
	feed->content = g_queue_new();
	feed->enabled = false;

	g_hash_table_insert(feeds, strdup(name), feed);

	return true;
}

/**
 * Adds a field to a feed
 *
 * @param name			the name of the feed to add the field to
 * @param field			the name of the field to add to the feed
 * @param expression	the expression for the field that should be added
 * @result				true if successful
 */
API bool addFeedField(const char *name, const char *field, const char *expression)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		LOG_ERROR("Failed to add field to feed '%s': A feed with that name doesn't exist", name);
		return false;
	}

	if(feed->enabled) {
		LOG_ERROR("Failed to add field to feed '%s': Feed is already enabled", name);
		return false;
	}

	if(g_hash_table_lookup(feed->fields, field) != NULL) {
		LOG_ERROR("Failed to add field '%s' to feed '%s': A field with that name already exists", name, field);
		return false;
	}

	g_hash_table_insert(feed->fields, strdup(field), strdup(expression));
	return true;
}

/**
 * Deletes a field from a feed
 *
 * @param name			the name of the feed to delete the field from
 * @param field			the name of the field to delete
 * @result				true if successful
 */
API bool deleteFeedField(const char *name, const char *field)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		LOG_ERROR("Failed to add field to feed '%s': A feed with that name doesn't exist", name);
		return false;
	}

	if(feed->enabled) {
		LOG_ERROR("Failed to add field to feed '%s': Feed is already enabled", name);
		return false;
	}

	return g_hash_table_remove(feed->fields, field);
}

/**
 * Enables a feed
 *
 * @param feed			the name of the feed to enable
 * @result				true if successful
 */
API bool enableFeed(const char *name)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		LOG_ERROR("Failed to enable feed '%s': A feed with that name doesn't exist", name);
		return false;
	}

	if(feed->enabled) {
		LOG_ERROR("Failed to enable feed '%s': Feed is already enabled", name);
		return false;
	}

	feed->enabled = true;
	return true;
}

/**
 * Returns the feed objects for a feed
 *
 * @param name			the name of the feed to retrieve
 * @result				the resulting feed of NULL on failure
 */
API Feed *getFeed(const char *name)
{
	Feed *feed;
	if((feed = g_hash_table_lookup(feeds, name)) == NULL) {
		LOG_ERROR("Failed to get feed '%s': A feed with that name doesn't exist", name);
		return NULL;
	}

	return feed;
}

/**
 * Deletes a feed
 *
 * @param name			the name of the feed that should be deleted
 * @result				true if successful
 */
API bool deleteFeed(const char *name)
{
	return g_hash_table_remove(feeds, name);
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
	free(feed->name);
	free(feed->url);
	g_hash_table_destroy(feed->fields);

	for(GList *iter = feed->content->head; iter != NULL; iter = iter->next) {
		g_hash_table_destroy(iter->data);
	}
	g_queue_free(feed->content);

	free(feed);
}
