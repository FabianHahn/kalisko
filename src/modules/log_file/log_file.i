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

#ifndef LOG_FILE_LOG_FILE_H
#define LOG_FILE_LOG_FILE_H

#include <stdio.h>

/**
 * Configuration information for a single log file.
 */
typedef struct {
	/**
	 * Location of the log file.
	 */
	char *filePath;

	/**
	 * The lowest log level to log into the log file.
	 */
	LogLevel level;

	/**
	 * FILE descriptor to append new lines.
	 */
	FILE *fileAppend;

	/**
	 * If this is true the next log entry has to be ignored. This is used by config.c to prevent endless
	 * loops in case of errors.
	 */
	bool ignoreNextLog;
} LogFileConfig;


/**
 * Adds a new LogFileConfig to the configuration files list. This can be used to add programmatically
 * additional log files.
 *
 * @param filePath	The path to the log file (directory path + file name)
 * @param level		The log level to use for the log file.
 * @return 			The created LogFileConfig or NULL on error
 */
API LogFileConfig *addLogFile(char *filePath, LogLevel level);

/**
 * Removes the given LogFileConfig from the list of log files and free it.
 *
 * @param logFile	The LogFileConfig to remove
 */
API void removeLogFile(LogFileConfig *logFile);

#endif
