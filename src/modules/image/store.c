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
#define API
#include "image.h"
#include "store.h"

API Image *createImageFromStore(Store *store)
{
	Store *width;
	if((width = $(Store *, store, getStorePath)(store, "image/width")) == NULL || width->type != STORE_INTEGER || width->content.integer <= 0) {
		logError("Failed to parse image store: Could not find store integer path 'image/width'");
		return NULL;
	}

	Store *height;
	if((height = $(Store *, store, getStorePath)(store, "image/height")) == NULL || height->type != STORE_INTEGER || height->content.integer <= 0) {
		logError("Failed to parse image store: Could not find store integer path 'image/height'");
		return NULL;
	}

	Store *type;
	if((type = $(Store *, store, getStorePath)(store, "image/type")) == NULL || type->type != STORE_STRING || (g_strcmp0(type->content.string, "byte") != 0 && g_strcmp0(type->content.string, "float") != 0)) {
		logError("Failed to parse image store: Store path 'image/type' must be either 'byte' or 'float'");
		return NULL;
	}

	Store *channels;
	if((channels = $(Store *, store, getStorePath)(store, "image/channels")) == NULL || channels->type != STORE_INTEGER || channels->content.integer <= 0) {
		logError("Failed to parse image store: Could not find store integer path 'image/channels'");
		return NULL;
	}

	Store *pixels;
	if((pixels = $(Store *, store, getStorePath)(store, "image/pixels")) == NULL || pixels->type != STORE_LIST) {
		logError("Failed to parse image store: Could not find store list path 'image/pixels'");
		return NULL;
	}

	GQueue *pList = pixels->content.list;

	Image *image;
	if(g_strcmp0(type->content.string, "byte") == 0) {
		image = createImageByte(width->content.integer, height->content.integer, channels->content.integer);
	} else {
		image = createImageFloat(width->content.integer, height->content.integer, channels->content.integer);
	}

	// Read pixels
	int x = 0;
	int y = 0;
	for(GList *iter = pList->head; iter != NULL; iter = iter->next) {
		Store *pixel = iter->data;

		if(pixel->type != STORE_LIST) {
			if(pixel->type == STORE_INTEGER) { // allow non-list format equal channel pixels (int)
				for(unsigned int c = 0; c < image->channels; c++) {
					setImage(image, x, y, c, pixel->content.integer / 255.0f);
				}
			} else if(pixel->type == STORE_FLOAT_NUMBER) { // allow non-list format equal channel pixels (float)
				for(unsigned int c = 0; c < image->channels; c++) {
					setImage(image, x, y, c, pixel->content.float_number);
				}
			} else {
				logWarning("Invalid pixel %d/%d in image store, setting to zero", x, y);
				for(unsigned int c = 0; c < image->channels; c++) {
					setImage(image, x, y, c, 0);
				}
			}
		} else {
			if(g_queue_get_length(pixel->content.list) != image->channels) {
				logWarning("Pixel %d/%d in image store has invalid number of %u channels, setting to zero", x, y, g_queue_get_length(pixel->content.list));
				for(unsigned int c = 0; c < image->channels; c++) {
					setImage(image, x, y, c, 0);
				}
			}

			int c = 0;
			for(GList *piter = pixel->content.list->head; piter != NULL; piter = piter->next, c++) {
				Store *pval = piter->data;

				if(pval->type == STORE_INTEGER && pval->content.integer >= 0 && pval->content.integer <= 255) {
					setImage(image, x, y, c, pval->content.integer / 255.0f);
				} else if(pval->type == STORE_FLOAT_NUMBER) {
					setImage(image, x, y, c, pval->content.float_number);
				} else {
					logWarning("Invalid value in channel %d of pixel %d/%d in image store, replacing by 0", c, x, y);
					setImage(image, x, y, c, 0);
				}
			}
		}

		x++;
		if(x >= image->width) {
			x = 0;
			y++;
		}
	}

	return image;
}

API Store *convertImageToStore(Image *image)
{
	Store *store = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(store, "image", $(Store *, store, createStore)());

	$(bool, store, setStorePath)(store, "image/width", $(Store *, store, createStoreIntegerValue)(image->width));
	$(bool, store, setStorePath)(store, "image/height", $(Store *, store, createStoreIntegerValue)(image->height));
	$(bool, store, setStorePath)(store, "image/channels", $(Store *, store, createStoreIntegerValue)(image->channels));

	switch(image->type) {
		case IMAGE_TYPE_BYTE:
			$(bool, store, setStorePath)(store, "image/type", $(Store *, store, createStoreStringValue)("byte"));
		break;
		case IMAGE_TYPE_FLOAT:
			$(bool, store, setStorePath)(store, "image/type", $(Store *, store, createStoreStringValue)("float"));
		break;
	}

	Store *pixels = $(Store *, store, createStoreListValue)(NULL);
	$(bool, store, setStorePath)(store, "image/pixels", pixels);

	// Write pixels
	for(unsigned int y = 0; y < image->height; y++) {
		for(unsigned int x = 0; x < image->width; x++) {
			Store *pixel = $(Store *, store, createStoreListValue)(NULL);

			for(unsigned int c = 0; c < image->channels; c++) {
				switch(image->type) {
					case IMAGE_TYPE_BYTE:
						g_queue_push_tail(pixel->content.list, $(Store *, store, createStoreIntegerValue)(getImageByte(image, x, y, c)));
					break;
					case IMAGE_TYPE_FLOAT:
						g_queue_push_tail(pixel->content.list, $(Store *, store, createStoreFloatNumberValue)(getImageFloat(image, x, y, c)));
					break;
				}
			}

			g_queue_push_tail(pixels->content.list, pixel);
		}
	}

	return store;
}
