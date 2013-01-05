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

#ifndef GETOPTS_GETOPTS_H
#define GETOPTS_GETOPTS_H


/**
 * Looks up an option and returns its supplied value if any was given or an empty string if the option was supplied without value.
 * Returns NULL if the option was not supplied at all.
 *
 * @param opt	The option to look up
 * @return		The value for the given option or an empty string or NULL
 */
API char *getOpt(char *opt);

/**
 * Looks up a list of options to check if they exist with a value. The first match of an option
 * is used to return the corresponding value. All other options are ignored.
 *
 * @param opt	A list of options to look up.
 * @return		The value for the first matched option or NULL
 */
API char *getOptValue(char *opt, ...) G_GNUC_NULL_TERMINATED;

/**
 * Dumps a string representations of all Opts and returns this string.
 *
 * Attention: Use this only for debugging.
 *
 * @return A string representing the parsed Opts. This string must be freed.
 */
API char *dumpOpts();

/**
 * Sets the internal "parsed" variable to the given value.
 *
 * Attention: Use this only for debugging / testing.
 */
API void setOptsParsed(bool v);

/**
 * Checks if the CLI option (OPT) exists. It does not have to have a value.
 *
 * @param OPT	The CLI option name as a string
 * @result		true if the CLI option exists.
 */
#define HAS_OPT(OPT) \
	($(char *, getopts, getOpt)(OPT) != NULL)

#endif
