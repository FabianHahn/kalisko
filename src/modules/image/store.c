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
#include "modules/store/path.h"
#include "api.h"
#include "image.h"
#include "store.h"

/**
 * Creates a image from a store
 *
 * @param store			the store to parse
 * @result				the parsed image of NULL on failure
 */
API Image *createImageFromStore(Store *store)
{
	Store *width;
	if((width = $(Store *, store, getStorePath)(store, "image/width")) == NULL || width->type != STORE_INTEGER || width->content.integer <= 0) {
		LOG_ERROR("Failed to parse image store: Could not find store integer path 'image/width'");
		return NULL;
	}

	Store *height;
	if((height = $(Store *, store, getStorePath)(store, "image/height")) == NULL || height->type != STORE_INTEGER || height->content.integer <= 0) {
		LOG_ERROR("Failed to parse image store: Could not find store integer path 'image/height'");
		return NULL;
	}

	Store *channels;
	if((channels = $(Store *, store, getStorePath)(store, "image/channels")) == NULL || channels->type != STORE_INTEGER || channels->content.integer <= 0) {
		LOG_ERROR("Failed to parse image store: Could not find store integer path 'image/channels'");
		return NULL;
	}

	Store *pixels;
	if((pixels = $(Store *, store, getStorePath)(store, "image/pixels")) == NULL || pixels->type != STORE_LIST) {
		LOG_ERROR("Failed to parse image store: Could not find store list path 'image/pixels'");
		return NULL;
	}

	GQueue *pList = pixels->content.list;

	Image *image = createImage(width->content.integer, height->content.integer, channels->content.integer);

	// Read pixels
	int i = 0;
	for(GList *iter = pList->head; iter != NULL; iter = iter->next, i++) {
		Store *pixel = iter->data;

		if(pixel->type != STORE_LIST || g_queue_get_length(pixel->content.list) != image->channels) {
			LOG_WARNING("Invalid pixel %d in image store, setting to zero", i);
			for(unsigned int c = 0; c < image->channels; c++) {
				image->data[i * image->channels + c] = 0.0f;
			}
		} else {
			int c = 0;
			for(GList *piter = pixel->content.list->head; piter != NULL; piter = piter->next, c++) {
				Store *pval = piter->data;

				switch(pval->type) {
					case STORE_FLOAT_NUMBER:
						image->data[i * image->channels + c] = pval->content.float_number;
					break;
					case STORE_INTEGER:
						image->data[i * image->channels + c] = pval->content.integer;
					break;
					default:
						LOG_WARNING("Invalid value in channel %d of pixel %d in image store, replacing by 0", c, i);
						image->data[i * image->channels + c] = 0.0f;
					break;
				}
			}
		}
	}

	return image;
}

/**
 * Creates a store from a image
 *
 * @param image		the image to convert to a store
 * @result			the converted store or NULL on failure
 */
API Store *convertImageToStore(Image *image)
{
	Store *store = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(store, "image", $(Store *, store, createStore)());

	$(bool, store, setStorePath)(store, "image/width", $(Store *, store, createStoreIntegerValue)(image->width));
	$(bool, store, setStorePath)(store, "image/height", $(Store *, store, createStoreIntegerValue)(image->height));
	$(bool, store, setStorePath)(store, "image/channels", $(Store *, store, createStoreIntegerValue)(image->channels));

	Store *pixels = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "image/pixels", pixels);

	// Write vertices
	for(unsigned int i = 0; i < image->width * image->height; i++) {
		Store *pixel = $(Store *, store, createStoreListValue)(NULL);

		for(unsigned int c = 0; c < image->channels; c++) {
			g_queue_push_tail(pixel->content.list, $(Store *, store, createStoreFloatNumberValue)(image->data[i * image->channels + c]));
		}

		g_queue_push_tail(pixels->content.list, pixel);
	}

	return store;
}
