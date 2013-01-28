/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, 2011, Kalisko Project Leaders
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
#include "memory_alloc.h"
#include "util.h"
#include "modules/config/config.h"
#include "modules/event/event.h"

#define API
#include "log_file.h"

#define LOG_FILES_CONFIG_PATH "kalisko/logfiles"
#define LOG_FILES_CONFIG_FILEPATH_KEY "filepath"
#define LOG_FILES_CONFIG_LOGTYPE_KEY "logtype"

#define LOG_FILES_LOGTYPE_DEBUG "debug"
#define LOG_FILES_LOGTYPE_INFO "info"
#define LOG_FILES_LOGTYPE_WARNING "warning"
#define LOG_FILES_LOGTYPE_ERROR "error"

#define LOGFILE_DIR_PERMISSION 0700

static void listener_log(void *subject, const char *event, void *data, va_list args);
static void finalize();

static GList *logFiles = NULL;

MODULE_NAME("log_file");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This log provider writes log messages to a user-defined file from the standard config");
MODULE_VERSION(0, 2, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("event", 0, 1, 2), MODULE_DEPENDENCY("log_event", 0, 1, 1));

MODULE_INIT
{
	// Go trough the standard configuration files and search for log file settings
	Store *configFiles = $(Store *, config, getConfigPath)(LOG_FILES_CONFIG_PATH);
	if(configFiles != NULL) {
		if(configFiles->type != STORE_LIST) {
			logWarning("Found log files configuration but it is not a list and can not be processed");
			return false;
		}
		for(int i = 0; i < configFiles->content.list->length; i++) {
			Store *fileConfig = g_queue_peek_nth(configFiles->content.list, i);
			if(fileConfig->type == STORE_ARRAY) {
				GHashTable *settings = fileConfig->content.array;
				Store *filePath = g_hash_table_lookup(settings, LOG_FILES_CONFIG_FILEPATH_KEY);
				Store *logType = g_hash_table_lookup(settings, LOG_FILES_CONFIG_LOGTYPE_KEY);

				if(filePath == NULL) {
					logWarning("The filepath is not set in the configuration. Ignoring log file");
					continue;
				} else if(logType == NULL) {
					logWarning("The logtype is not set in the configuration. Ignoring log file");
					continue;
				}

				if(filePath->type != STORE_STRING) {
					logWarning("The filepath is not a string. Ignoring log file");
					continue;
				} else if(logType->type != STORE_STRING) {
					logWarning("The logtype is not a string. Ignoring log file");
					continue;
				}

				// parse the string value to the needed type
				LogLevel level = LOG_LEVEL_INFO;
				if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_DEBUG) == 0) {
					level = LOG_LEVEL_INFO;
				} else if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_INFO) == 0) {
					level = LOG_LEVEL_NOTICE;
				} else if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_WARNING) == 0) {
					level = LOG_LEVEL_WARNING;
				} else if(strcmp(logType->content.string, LOG_FILES_LOGTYPE_ERROR) == 0) {
					level = LOG_LEVEL_ERROR;
				} else {
					logWarning("Could not interpret logtype value: %s",logType->content.string);
					continue;
				}

				addLogFile(filePath->content.string, level);
			} else {
				logWarning("Found list of log file configurations but one of the elements is not an array");
			}
		}
	}

	$(void, event, attachEventListener)(NULL, "log", NULL, &listener_log);

	return true;
}

MODULE_FINALIZE
{
	finalize();
}

API LogFileConfig *addLogFile(char *filePath, LogLevel level)
{
	LogFileConfig *logFile = ALLOCATE_OBJECT(LogFileConfig);
	logFile->filePath = strdup(filePath);
	logFile->level = level;
	logFile->ignoreNextLog = false;
	logFile->fileAppend = NULL;

	// checking the directory exists and if not try to create it
	char *dirPath = $$(char *, getDirectoryPath)(logFile->filePath);
	if(!g_file_test(dirPath, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) && !g_mkdir_with_parents(dirPath, LOGFILE_DIR_PERMISSION)) {
		logError("Could not create parent directory for the log file '%s'.", logFile->filePath);
		removeLogFile(logFile);

		return NULL;
	}

	free(dirPath);

	logFiles = g_list_append(logFiles, logFile);
	return logFile;
}

API void removeLogFile(LogFileConfig *logFile)
{
	logFiles = g_list_remove(logFiles, logFile);

	if(logFile->fileAppend) {
		fclose(logFile->fileAppend);
	}

	free(logFile->filePath);
	free(logFile);
}

static void listener_log(void *subject, const char *event, void *data, va_list args)
{
	const char *module = va_arg(args, const char *);
	LogLevel level = va_arg(args, LogLevel);
	char *message = va_arg(args, char *);

	GDateTime *now = g_date_time_new_now_local();
	unsigned int day = g_date_time_get_day_of_month(now);
	unsigned int month = g_date_time_get_month(now);
	unsigned int year = g_date_time_get_year(now);
	unsigned int hour = g_date_time_get_hour(now);
	unsigned int minute = g_date_time_get_minute(now);
	unsigned int second = g_date_time_get_second(now);
    g_date_time_unref(now);

    for(GList *item = logFiles; item != NULL; item = item->next) {
    	LogFileConfig *logFile = item->data;

    	if(logFile->ignoreNextLog) {
    		logFile->ignoreNextLog = false;
    		continue;
    	}

    	if(logFile->fileAppend == NULL) {
    		if((logFile->fileAppend = fopen(logFile->filePath, "a")) == NULL) {
    			logFile->ignoreNextLog = true;
    			logWarning("Could not open logfile: %s", logFile->filePath);

    			continue;
    		}
    	}

    	switch(logFile->level) {
			case LOG_LEVEL_INFO:
				if(level == LOG_LEVEL_INFO)
					fprintf(logFile->fileAppend, "[%02u.%02u.%04u-%02u:%02u:%02u] [%s] NOTICE: %s\n", day, month, year, hour, minute, second, module, message);
			case LOG_LEVEL_NOTICE:
				if(level == LOG_LEVEL_NOTICE)
					fprintf(logFile->fileAppend, "[%02u.%02u.%04u-%02u:%02u:%02u] [%s] INFO: %s\n", day, month, year, hour, minute, second, module, message);
			case LOG_LEVEL_WARNING:
				if(level == LOG_LEVEL_WARNING)
					fprintf(logFile->fileAppend, "[%02u.%02u.%04u-%02u:%02u:%02u] [%s] WARNING: %s\n", day, month, year, hour, minute, second, module, message);
			case LOG_LEVEL_ERROR:
				if(level == LOG_LEVEL_ERROR)
					fprintf(logFile->fileAppend, "[%02u.%02u.%04u-%02u:%02u:%02u] [%s] ERROR: %s\n", day, month, year, hour, minute, second, module, message);
			default:
			break;
    	}

    	fflush(logFile->fileAppend);
    }
}

static void finalize()
{
	$(void, event, detachEventListener)(NULL, "log", NULL, &listener_log);

	if(logFiles) {
		for(GList *item = logFiles; item != NULL; item = item->next) {
			removeLogFile(item->data);
		}

		g_list_free(logFiles);
	}
}
