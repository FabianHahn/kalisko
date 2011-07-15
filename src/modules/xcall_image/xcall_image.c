/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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
#include "modules/store/merge.h"
#include "modules/xcall/xcall.h"
#include "modules/image/io.h"
#include "modules/image/store.h"
#include "api.h"

static Store *xcall_readImageFile(Store *xcall);
static Store *xcall_writeImageFile(Store *xcall);

MODULE_NAME("xcall_image");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("XCall module for images");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("image", 0, 5, 16), MODULE_DEPENDENCY("xcall", 0, 2, 6));

MODULE_INIT
{
	bool fail = true;

	do {
		if(!$(bool, xcall, addXCallFunction)("readImageFile", &xcall_readImageFile)) {
			break;
		}

		if(!$(bool, xcall, addXCallFunction)("writeImageFile", &xcall_writeImageFile)) {
			break;
		}

		fail = false;
	}
	while(false);

	if(fail) {
		$(bool, xcall, delXCallFunction)("readImageFile");
		$(bool, xcall, delXCallFunction)("writeImageFile");
	}

	return true;
}

MODULE_FINALIZE
{
	$(bool, xcall, delXCallFunction)("readImageFile");
	$(bool, xcall, delXCallFunction)("writeImageFile");
}

/**
 * XCallFunction to read a  image from a file
 * XCall parameters:
 *  * string file			the filename of the image to read
 * XCall result:
 * 	* array image			the parsed image
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_readImageFile(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());

	Store *file;
	if((file = $(Store *, store, getStorePath)(xcall, "file")) == NULL || file->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'file'"));
		return ret;
	}

	Image *image;
	if((image = $(Image *, imageio, readImageFromFile)(file->content.string)) == NULL) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read image from specified file"));
		return ret;
	}

	Store *imagestore;
	if((imagestore = $(Store *, image_store, convertImageToStore)(image)) == NULL) {
		$(void, opengl, freeImage)(image);
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to convert image to store"));
		return ret;
	}

	$(bool, store, mergeStore)(ret, imagestore);
	$(void, store, freeStore)(imagestore);

	return ret;
}

/**
 * XCallFunction to write a image to a file
 * XCall parameters:
 *  * string file 		the filename of the image to write
 *  * array image		the image to write to the file
 * XCall result:
 * 	* int success		nonzero if successful
 *
 * @param xcall		the xcall as store
 * @result			a return value as store
 */
static Store *xcall_writeImageFile(Store *xcall)
{
	Store *ret = $(Store *, store, createStore)();
	$(bool, store, setStorePath)(ret, "xcall", $(Store *, store, createStore)());

	Store *file;
	if((file = $(Store *, store, getStorePath)(xcall, "file")) == NULL || file->type != STORE_STRING) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory string parameter 'file'"));
		return ret;
	}

	Store *imagestore;
	if((imagestore = $(Store *, store, getStorePath)(xcall, "image")) == NULL || imagestore->type != STORE_ARRAY) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to read mandatory array parameter 'image'"));
		return ret;
	}

	Image *image;
	if((image = $(Image *, image_store, createImageFromStore)(xcall)) == NULL) {
		$(bool, store, setStorePath)(ret, "xcall/error", $(Store *, store, createStoreStringValue)("Failed to create image from store"));
		return ret;
	}

	$(bool, store, setStorePath)(ret, "success", $(Store *, store, createStoreIntegerValue)($(bool, imageio, writeImageToFile)(image, file->content.string)));
	$(void, opengl, freeImage)(image);

	return ret;
}
