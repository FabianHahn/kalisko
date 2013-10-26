/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * Copyright (c) 2013, Google Inc.
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

#ifndef RPC_RPC_H
#define RPC_RPC_H

#include <glib.h>
#include <stdarg.h>
#include "modules/socket/socket.h"
#include "modules/store/store.h"

/** Type of function which can be used to implement an RPC stub. */
typedef Store* (*RpcImplementation) (Store *);

/** 
 * Makes an RPC implementation available to clients. Does not take ownership of any passed parameters.
 * @param path the path of the RPC on this server
 * @param request_schema    a schema to validate request stores passed into the implementation
 * @param response_schema   a schema used to validate the result of calling the implementation
 * @param implementation    a function to be called when an rpc occurs. The function may assume that the request
 *                          is valid according to the request schema and must produce a valid response.
 * @return                  whether or not registering the rpc was successful.
 */
API bool registerRpc(char *path, Store *request_schema, Store *response_schema, RpcImplementation implementation);

/**
 * Unregisters a previously registered rpc, making it no longer available to clients.
 * @param path    the path of the rpc to unregister
 * @return        whether or not unregistering was successful
 */
API bool unregisterRpc(char *path);

/**
 * Makes a local RPC call, acting as if a client had opened a connection and executed "rpc call <path>\n <request>\n\n".
 * @param path      the path the rpc to call
 * @param request   the request store passed to the rpc. Must be valid according to the rpc request schema
 * @return          a new valid store according to the rpc response schema or NULL if the rpc failed. The 
 *                  caller takes responsability to call freeStore on the result. 
 */
API Store *callRpc(char *path, Store *request);

#endif
