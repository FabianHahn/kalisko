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

#ifndef FEED_FEED_H
#define FEED_FEED_H

typedef struct {
	/** The name of the field */
	char *name;
	/** The XPath expression for the field */
	char *expression;
} FeedField;

typedef struct {
	/** The name of the feed */
	char *name;
	/** The URL of the feed */
	char *url;
	/** The fields of the feed consisting of FeedField objects */
	GQueue *fields;
	/** The content list of the feed consisting of GHashTable items associating field names with field values */
	GQueue *content;
	/** Whether the feed is enabled */
	bool enabled;
} Feed;


/**
 * Creates a feed
 *
 * @param name			the name of the feed
 * @param url			the URL of the feed
 * @result				true if successful
 */
API bool createFeed(const char *name, const char *url);

/**
 * Adds a field to a feed
 *
 * @param name			the name of the feed to add the field to
 * @param fieldName		the name of the field to add to the feed
 * @param expression	the expression for the field that should be added
 * @result				true if successful
 */
API bool addFeedField(const char *name, const char *fieldName, const char *expression);

/**
 * Deletes a field from a feed
 *
 * @param name			the name of the feed to delete the field from
 * @param fieldName		the name of the field to delete
 * @result				true if successful
 */
API bool deleteFeedField(const char *name, const char *fieldName);

/**
 * Enables a feed
 *
 * @param feed			the name of the feed to enable
 * @result				true if successful
 */
API bool enableFeed(const char *name);

/**
 * Returns the feed objects for a feed
 *
 * @param name			the name of the feed to retrieve
 * @result				the resulting feed of NULL on failure
 */
API Feed *getFeed(const char *name);

/**
 * Deletes a feed
 *
 * @param name			the name of the feed that should be deleted
 * @result				true if successful
 */
API bool deleteFeed(const char *name);

#endif
