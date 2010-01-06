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

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>

#include "dll.h"
#include "log.h"
#include "types.h"
#include "hooks.h"
#include "memory_alloc.h"
#include "util.h"
#include "modules/config/config.h"
#include "modules/config/path.h"
#include "modules/config_standard/util.h"

#include "api.h"
#include "log_file.h"

#define LOG_FILES_CONFIG_PATH "logfiles"
#define LOG_FILES_CONFIG_FILEPATH_KEY "filepath"
#define LOG_FILES_CONFIG_LOGTYPE_KEY "logtype"

#define LOG_FILES_LOGTYPE_DEBUG "debug"
#define LOG_FILES_LOGTYPE_INFO "info"
#define LOG_FILES_LOGTYPE_WARNING "warning"
#define LOG_FILES_LOGTYPE_ERROR "error"

#define LOGFILE_DIR_PERMISSION 0700

HOOK_LISTENER(log);
static void finalize();

static GList *logFiles = NULL;

MODULE_NAME("log_file");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This log provider writes log messages to a user-defined file from the standard config");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config_standard", 0, 1, 0));

MODULE_INIT
{
	// Go trough the standard configuration files and search for log file settings
	ConfigNodeValue *configFiles = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)(LOG_FILES_CONFIG_PATH);
	if(configFiles != NULL) {
		if(configFiles->type != CONFIG_LIST) {
			LOG_WARNING("Found log files configuration but it is not a list and can not be processed");
			return false;
		}
		for(int i = 0; i < configFiles->content.list->length; i++) {
			ConfigNodeValue *fileConfig = g_queue_peek_nth(configFiles->content.list, i);
			if(fileConfig->type == CONFIG_ARRAY) {
				GHashTable *settings = fileConfig->content.array;
				ConfigNodeValue *filePath = g_hash_table_lookup(settings, LOG_FILES_CONFIG_FILEPATH_KEY);
				ConfigNodeValue *logType = g_hash_table_lookup(settings, LOG_FILES_CONFIG_LOGTYPE_KEY);

				if(filePath == NULL) {
					LOG_WARNING("The filepath is not set in the configuration. Ignoring log file");
					continue;
				} else if(logType == NULL) {
					LOG_WARNING("The logtype is not set in the configuration. Ignoring log file");
					continue;
				}

				if(filePath->type != CONFIG_STRING) {
					LOG_WARNING("The filepath is not a string. Ignoring log file");
					continue;
				} else if(logType->type != CONFIG_STRING) {
					LOG_WARNING("The logtype is not a string. Ignoring log file");
					continue;
				}

				// parse the string value to the needed type
				LogType parsedLogtype;
				if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_DEBUG) == 0) {
					parsedLogtype = LOG_TYPE_DEBUG;
				} else if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_INFO) == 0) {
					parsedLogtype = LOG_TYPE_INFO;
				} else if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_WARNING) == 0) {
					parsedLogtype = LOG_TYPE_WARNING;
				} else if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_ERROR) == 0) {
					parsedLogtype = LOG_TYPE_ERROR;
				} else {
					LOG_WARNING("Could not interpret logtype value: %s",logType->content.string);
					continue;
				}

				addLogFile(filePath->content.string, parsedLogtype);
			} else {
				LOG_WARNING("Found list of log file configurations but one of the elements is not an array");
			}
		}
	}

	if(!HOOK_ATTACH(log, log)) {
		finalize();
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	finalize();
}

/**
 * Adds a new LogFileConfig to the configuration files list. This can be used to add programmatically
 * additional log files.
 *
 * @param filePath	The path to the log file (directory path + file name)
 * @param logType	The LogType to use for the log file. This means that all messages of this type or a higher one are written into
 * 					the file
 * @return The created LogFileConfig or NULL on error
 */
API LogFileConfig *addLogFile(char *filePath, LogType logType)
{
	LogFileConfig *logFile = ALLOCATE_OBJECT(LogFileConfig);
	logFile->filePath = strdup(filePath);
	logFile->logType = logType;
	logFile->ignoreNextLog = false;
	logFile->fileAppend = NULL;

	// checking the directory exists and if not try to create it
	char *dirPath = $$(char *, getDirectoryPath)(logFile->filePath);
	if(!g_file_test(dirPath, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) && !g_mkdir_with_parents(dirPath, LOGFILE_DIR_PERMISSION)) {
		LOG_ERROR("Could not create parent directory for the log file '%s'.", logFile->filePath);
		removeLogFile(logFile);

		return NULL;
	}

	free(dirPath);

	logFiles = g_list_append(logFiles, logFile);
	return logFile;
}

/**
 * Removes the given LogFileConfig from the list of log files and free it.
 *
 * @param logFile	The LogFileConfig to remove
 */
API void removeLogFile(LogFileConfig *logFile)
{
	logFiles = g_list_remove(logFiles, logFile);

	if(logFile->fileAppend) {
		fclose(logFile->fileAppend);
	}

	free(logFile->filePath);
	free(logFile);
}

HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

    for(GList *item = logFiles; item != NULL; item = item->next) {
    	LogFileConfig *logFile = item->data;

    	if(logFile->ignoreNextLog) {
    		logFile->ignoreNextLog = false;
    		continue;
    	}

    	if(logFile->fileAppend == NULL) {
    		if((logFile->fileAppend = fopen(logFile->filePath, "a")) == NULL) {
    			logFile->ignoreNextLog = true;
    			LOG_WARNING("Could not open logfile: %s", logFile->filePath);

    			continue;
    		}
    	}

    	switch(logFile->logType) {
			case LOG_TYPE_DEBUG:
				if(type == LOG_TYPE_DEBUG)
					fprintf(logFile->fileAppend, "%s DEBUG: %s\n", dateTime, message);
			case LOG_TYPE_INFO:
				if(type == LOG_TYPE_INFO)
					fprintf(logFile->fileAppend, "%s INFO: %s\n", dateTime, message);
			case LOG_TYPE_WARNING:
				if(type == LOG_TYPE_WARNING)
					fprintf(logFile->fileAppend, "%s WARNING: %s\n", dateTime, message);
			case LOG_TYPE_ERROR:
				if(type == LOG_TYPE_ERROR)
					fprintf(logFile->fileAppend, "%s ERROR: %s\n", dateTime, message);
    	}
    }

    free(now);
    free(dateTime);
}

static void finalize()
{
	HOOK_DETACH(log, log);

	if(logFiles) {
		for(GList *item = logFiles; item != NULL; item = item->next) {
			removeLogFile(item->data);
		}

		g_list_free(logFiles);
	}
}
