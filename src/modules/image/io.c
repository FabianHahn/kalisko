/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "modules/store/store.h"
#include "modules/store/parse.h"
#include "modules/store/write.h"
#define API
#include "image.h"
#include "store.h"
#include "io.h"

static Image *readImageStore(const char *filename);
static bool writeImageStore(const char *filename, Image *image);

/**
 * A hash table associating strings with ImageIOReadHandlers
 */
static GHashTable *readHandlers;

/**
 * A hash table associating strings with ImageIOWriteHandlers
 */
static GHashTable *writeHandlers;

API void initImageIO()
{
	readHandlers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);
	writeHandlers = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, NULL);

	addImageIOReadHandler("store", &readImageStore);
	addImageIOWriteHandler("store", &writeImageStore);
}

API void freeImageIO()
{
	g_hash_table_destroy(readHandlers);
	g_hash_table_destroy(writeHandlers);
}

API bool addImageIOReadHandler(const char *extension, ImageIOReadHandler *handler)
{
	if(g_hash_table_lookup(readHandlers, extension) != NULL) {
		logError("Trying to add image IO reading handler for already handled extension '%s'", extension);
		return false;
	}

	g_hash_table_insert(readHandlers, strdup(extension), handler);
	return true;
}

API bool deleteImageIOReadHandler(const char *extension)
{
	return g_hash_table_remove(readHandlers, extension);
}

API Image *readImageFromFile(const char *filename)
{
	if(!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		logError("Trying to read image from non existing file '%s'", filename);
		return NULL;
	}

	char *ext;

	if((ext = g_strrstr(filename, ".")) == NULL) {
		logError("Trying to read image from extensionless file '%s'", filename);
		return NULL;
	}

	ext++; // move past the dot

	ImageIOReadHandler *handler;
	if((handler = g_hash_table_lookup(readHandlers, ext)) == NULL) {
		logError("Tried to read image file '%s', but no handler was found for the extension '%s'", filename, ext);
		return NULL;
	}

	// We found a handler for this extension, so let it handle the reading
	return handler(filename);
}


API bool addImageIOWriteHandler(const char *extension, ImageIOWriteHandler *handler)
{
	if(g_hash_table_lookup(writeHandlers, extension) != NULL) {
		logError("Trying to add image IO writing handler for already handled extension '%s'", extension);
		return false;
	}

	g_hash_table_insert(writeHandlers, strdup(extension), handler);
	return true;
}

API bool deleteImageIOWriteHandler(const char *extension)
{
	return g_hash_table_remove(writeHandlers, extension);
}

API bool writeImageToFile(Image *image, const char *filename)
{
	char *ext;

	if((ext = g_strrstr(filename, ".")) == NULL) {
		logError("Trying to write image to extensionless file '%s'", filename);
		return false;
	}

	ext++; // move past the dot

	ImageIOWriteHandler *handler;
	if((handler = g_hash_table_lookup(writeHandlers, ext)) == NULL) {
		logError("Tried to write image to file '%s', but no handler was found for the extension '%s'", filename, ext);
		return false;
	}

	// We found a handler for this extension, so let it handle the reading
	return handler(filename, image);
}

/**
 * Reads a image from a store file
 *
 * @param filename			the store file to read from
 * @result					the parsed image of NULL on failure
 */
static Image *readImageStore(const char *filename)
{
	Store *store;
	if((store = $(Store *, store, parseStoreFile)(filename)) == NULL) {
		logError("Failed to parse image store file '%s'", filename);
		return NULL;
	}

	Image *image = createImageFromStore(store);

	$(void, store, freeStore)(store);

	return image;
}

/**
 * Writes an OpenGL image to a store file
 *
 * @param filename			the file name of the image store file to be saved
 * @param image				the OpenGL image to be written
 * @result					true if successful
 */
static bool writeImageStore(const char *filename, Image *image)
{
	Store *store = convertImageToStore(image);
	bool result = $(bool, store, writeStoreFile)(filename, store);
	$(void, store, freeStore)(store);

	return result;
}
