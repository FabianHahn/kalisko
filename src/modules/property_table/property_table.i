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

#ifndef PROPERTY_TABLE_H
#define PROPERTY_TABLE_H


/**
 * Returns the value found in the table for the given subject and corresponding
 * to the given key.
 *
 * If no value exists NULL is returned.
 *
 * @param subject	The subject to find the corresponding subject specific table
 * @param key		The key to find the value in the table
 * @return NULL if no value was found otherwise the value
 */
API void *getPropertyTableValue(void *subject, char *key);

/**
 * Sets, replaces or deletes the given value in the subject specific table using the given key.
 *
 * @param subject	The subject to find the corresponding subject specific table
 * @param key		The key to find the value in the table
 * @param value		The value to set or NULL if the key-value pair should be removed
 */
API void setPropertyTableValue(void *subject, char *key, void *value);

/**
 * Frees the table corresponding to the given subject.
 *
 * @param subject	The subject for which the table has to be freed.
 */
API void freePropertyTable(void *subject);

/**
 * Dumps all tables and their content into a string. It should only be used for
 * testing proposes.
 *
 * @return The dump as a string. This string must be freed.
 */
API char *dumpPropertyTables();


#endif
