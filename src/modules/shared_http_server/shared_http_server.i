/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2013, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *		 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *		 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *			 in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SHAREDHTTPSERVER_SHAREDHTTPSERVER_H
#define SHAREDHTTPSERVER_SHAREDHTTPSERVER_H

/**
 * Adds an HTTP request handler to the shared server which is executed upon requests matching <prefix>/<regexp>.
 * In order to determine the matching precedence, matches are tested in the order in which they were registered. 
 * Note that the caller retains ownership of all passed parameters (the regexp is copied). It is *NOT* necessary
 * to unregister every handler remaining handlers are removed automatically if and when the server shuts down.
 *
 * @param prefix				a prefix for the URL (usually identifies the Kalisko module)
 * @param hierarchical_regexp	the regular expression used to determine whether the request matches
 * @param handler				a handler function to be called for matching requests
 * @param userdata				custom userdata passed to the handler
 */
API void registerSharedHttpServerRequestHandlerWithPrefix(char *prefix, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata);

/**
 * Removes a registered request handler. Does nothing if the parameters do not match any handler.
 *
 * @param prefix				a prefix for the URL (usually identifies the Kalisko module)
 * @param hierarchical_regexp	the regular expression passed to register the handler
 * @param handler				the handler function passed at registration time
 * @param userdata				the userdata passed at registration time
 */
API void unregisterSharedHttpServerRequestHandlerWithPrefix(char *prefix, char *hierarchical_regexp, HttpRequestHandler *handler, void *userdata);

/**
 * Adds an HTTP request handler to the shared server which is executed upon requests matching KALISKO_MODULE/<regexp>.
 * In order to determine the matching precedence, matches are tested in the order in which they were registered. 
 * Note that the caller retains ownership of all passed parameters (the regexp is copied). It is *NOT* necessary
 * to unregister every handler remaining handlers are removed automatically if and when the server shuts down.
 *
 * @param hierarchical_regexp	the regular expression used to determine whether the request matches
 * @param handler				a handler function to be called for matching requests
 * @param userdata				custom userdata passed to the handler
 */
#define registerSharedHttpServerRequestHandler(hierarchical_regexp, handler, userdata) registerSharedHttpServerRequestHandlerWithPrefix(STR(KALISKO_MODULE), hierarchical_regexp, handler, userdata)

/**
 * Removes a registered request handler for the current Kalisko module. Does nothing if the parameters do not match any handler.
 *
 * @param hierarchical_regexp	the regular expression passed to register the handler
 * @param handler				the handler function passed at registration time
 * @param userdata				the userdata passed at registration time
 */
#define unregisterSharedHttpServerRequestHandler(hierarchical_regexp, handler, userdata) unregisterSharedHttpServerRequestHandlerWithPrefix(STR(KALISKO_MODULE), hierarchical_regexp, handler, userdata)

#endif