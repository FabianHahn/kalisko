/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
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

#ifndef STORE_PATH_H
#define STORE_PATH_H


/**
 * Fetches a store value by its path
 *
 * @param parent		the store in which the lookup takes place
 * @param pathFormat	the printf style path to the value without a leading / to search, use integers from base 0 for list elements
 * @result				the store value, or NULL if not found
 */
API Store *getStorePath(Store *store, const char *pathFormat, ...) G_GNUC_PRINTF(2, 3);

/**
 * Sets a value in a store path
 *
 * @param store			the store to edit
 * @param pathFormat	the printf style path to set, will be overridden if already exists
 * @param value			the value to set
 * @result				true if successful
 */
API bool setStorePath(Store *store, char *pathFormat, void *value, ...) G_GNUC_PRINTF(2, 4);

/**
 * Deletes a value in a store path
 *
 * @param store	the store to edit
 * @param path		the path to delete
 * @result			true if successful
 */
API bool deleteStorePath(Store *store, char *path);

/**
 * Splits a store path by its unescaped delimiter '/'
 *
 * @param path		the path to escape
 * @result			an array of path elements, contents must be freed with free and the array itself with g_ptr_array_free
 */
API GPtrArray *splitStorePath(char *path);

#endif
