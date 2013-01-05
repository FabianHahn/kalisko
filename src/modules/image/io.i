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

#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include "image.h"

typedef Image *(ImageIOReadHandler)(const char *filename);
typedef bool (ImageIOWriteHandler)(const char *filename, Image *image);


/**
 * Initializes the image IO system
 */
API void initImageIO();

/**
 * Frees the image IO system
 */
API void freeImageIO();

/**
 * Adds a image IO reading handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addImageIOReadHandler(const char *extension, ImageIOReadHandler *handler);

/**
 * Removes a image IO reading handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteImageIOReadHandler(const char *extension);

/**
 * Reads a image from a file by using the appropriate handler
 *
 * @param filename			the image file that should be loaded
 * @result					the loaded image or NULL on error
 */
API Image *readImageFromFile(const char *filename);

/**
 * Adds a image IO writing handler for a specific file extension
 *
 * @param extension			the file extension to which this handler should be registered
 * @param handler			the handler to register
 * @result					true if successful
 */
API bool addImageIOWriteHandler(const char *extension, ImageIOWriteHandler *handler);

/**
 * Removes a image IO writing handler from a specific file extension
 *
 * @param extension			the file extension for which the handler should be unregistered
 * @result					true if successful
 */
API bool deleteImageIOWriteHandler(const char *extension);

/**
 * Writes an OpenGL image to a file by using the appropriate handler
 *
 * @param image				the image to be written
 * @param filename			the file into which the image should be written
 * @result					true if successful
 */
API bool writeImageToFile(Image *image, const char *filename);

#endif
