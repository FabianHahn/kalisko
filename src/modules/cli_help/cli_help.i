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


#ifndef HELP_OPT_HELP_OPT_H
#define HELP_OPT_HELP_OPT_H


/**
 * Adds a new help entry for the given short / long command line option.
 *
 * Altough shortOpt and longOpt are optional one of them must be given.
 * All strings must be \0 terminated and encoded in UTF-8 (and because of that
 * also ASCII is allowed).
 *
 * @param moduleName	The name of the module owning the option. Required.
 * @param shortOpt		The short option without a prepended dash (so without the '-'). Optional.
 * @param longOpt		The long option without a prepended double dash (so without the '--'). Optional.
 * @param briefHelp		The short help. Required.
 * @return True if the help was added, false on error.
 */
API bool addCLOptionHelp(char *moduleName, char *shortOpt, char *longOpt, char *briefHelp);

/**
 * Adds a new help entry for the given command line argument.
 *
 * All strings must be \0 terminated and encoded in UTF-8 (and because of that
 * also ASCII is allowed).
 *
 * @param moduleName	The name of the module owning the argument. Required.
 * @param name			The name of the argument. Required.
 * @param briefHelp		The short help. Required.
 * @return True if the help was added, false on error.
 */
API bool addCLArgumentHelp(char *moduleName, char *name, char *briefHelp);

#endif
