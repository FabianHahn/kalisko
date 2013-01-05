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

#ifndef UTIL_H
#define UTIL_H


/**
 * Returns the path to the core executable
 *
 * @result		the executable's path
 */
API char *getExecutablePath();

/**
 * Returns the path for the parents directory of a file path (removes the file from the path).
 *
 * @param filePath	The path to a file
 * @return The path to the parents directory. Must be freed
 */
API char *getDirectoryPath(char *filePath);

/**
 * A GCompareDataFunc for integers
 *
 * @param a		the first number to compare
 * @param b		the second number to compare
 * @param data	unused
 * @result		negative if a < b, zero if a = b, positive if a > b
 */
API int compareIntegers(const void *a, const void *b, void *data);

/**
 * A GCompareDataFunc for GTimeVals
 *
 * @param a		the first time to compare
 * @param b		the second time to compare
 * @param data	unused
 * @result		negative if a < b, zero if a = b, positive if a > b
 */
API int compareTimes(const void *a, const void *b, void *data);

/**
 * Returns the argv from main().
 *
 * @return The argv from main().
 */
API char **getArgv();

/**
 * Sets the argv.
 *
 * @param args		The argv from main().
 */
API void setArgv(char **args);

/**
 * Returns the argc from main().
 *
 * @return The argc from main().
 */
API int getArgc();

/**
 * Sets the argc.
 *
 * @param count	The argc from main().
 */
API void setArgc(int count);

/**
 * Utility function to set breakpoints if the debugger doesn't support setting module breakpoints from the beginning of execution
 * Usage: Just call this function whereever you'd like to set a breakpoint inside a module but set the breakpoint in your debugger HERE
 */
API void breakpoint();

/**
 * Returns the current time in seconds with microsecond precision
 *
 * @result		the current time in seconds with microsecond precision
 */
API double getMicroTime();

/**
 * Returns the current time in seconds
 *
 * @result		the current time in seconds
 */
API int getTime();

/**
 * Returns the executable name.
 *
 * @result		the name of the executable. Must be freed after use.
 */
API char *getExecutableName();

#endif
