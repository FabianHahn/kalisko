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


#ifndef XCALL_XCALL_H
#define XCALL_XCALL_H

#include "modules/store/store.h"

/**
 * Function pointer type for an XCall function
 */
typedef Store *(XCallFunction)(Store *xcall);


/**
 * Adds a new XCall function
 *
 * @param name		the name of the hook
 * @param func		the xcall to add
 * @result			true if successful, false if the xcall already exists
 */
API bool addXCallFunction(const char *name, XCallFunction *func);

/**
 * Deletes an existing xcall
 *
 * @param name		the name of the xcall
 * @result			true if successful, if the xcall was not found
 */
API bool delXCallFunction(const char *name);

/**
 * Checks whether an XCall function exists
 *
 * @param name		the name of the xcall function
 * @result			true if it exists
 */
API bool existsXCallFunction(const char *name);

/**
 * Invokes an xcall
 *
 * @param xcall		a store containing the xcall
 * @result			a store containing the result of the xcall, must be freed by the caller with freeStore
 */
API Store *invokeXCall(Store *xcall) G_GNUC_WARN_UNUSED_RESULT;

/**
 * Invokes an xcall by a string store
 *
 * @param xcallstr	a store string containing the xcall
 * @result			a store containing the result of the xcall, must be freed by the caller with freeStore
 */
API Store *invokeXCallByString(const char *xcallstr) G_GNUC_WARN_UNUSED_RESULT;

#endif
