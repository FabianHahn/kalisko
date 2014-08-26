/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2014, Kalisko Project Leaders
 * Copyright (c) 2014, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *       @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *       @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *           in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JAVAMODULE_JAVAMODULE_H
#define JAVAMODULE_JAVAMODULE_H

#include "types.h"

/**
 * Looks for a class with the given name, instantiates it and starts by calling its init method on
 * the current thread. Then, it calls its run() method on a separate, dedicated thread. The
 * specified class have a default constructor and implement the org.kalisko.core.Module interface.
 *
 * @param moduleClass    the fully qualified class name (including its package)
 * @result               whether or not loading the Java module was successful
 */
API bool executeJavaModule(char *moduleClass);

#endif
