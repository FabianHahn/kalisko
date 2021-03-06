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


#ifndef MODULE_UTIL_MODULE_UTIL_H
#define MODULE_UTIL_MODULE_UTIL_H


/**
 * Safely revokes a module inside a timer callback, so there's no risk of accidentally unloading the caller before the revoke call completes.
 * Note that the only module this function isn't able to revoke is this module itself, module_util. Use the classic revokeModule function to remove this module, but do it in a safe environment!
 *
 * @param name			the name of the module to revoke
 */
API void safeRevokeModule(char *name);

/**
 * Safely force unloads a module inside a timer callback, so there's no risk of accidentally unloading the caller before the revoke call completes.
 * Note that the only module this function isn't able to revoke is this module itself, module_util. Use the classic forceUnloadModule function to remove this module, but do it in a safe environment!
 *
 * @param name			the name of the module to revoke
 */
API void safeForceUnloadModule(char *name);

/**
 * Safely force reloads a module inside a timer callback, so there's no risk of accidentally unloading the caller before the revoke call completes.
 * Note that the only module this function isn't able to revoke is this module itself, module_util. Use the classic forceUnloadModule function to remove this module, but do it in a safe environment!
 *
 * @param name			the name of the module to reload
 */
API void safeForceReloadModule(char *name);

#endif
