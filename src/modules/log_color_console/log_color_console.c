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


#include <stdlib.h>
#include <stdio.h> // fprintf
#include <glib.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "dll.h"
#include "hooks.h"
#include "modules/config/config.h"
#include "modules/config/path.h"
#include "modules/config_standard/util.h"

#include "log.h"
#include "api.h"

MODULE_NAME("log_color_console");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Kalisko console log provider with colored output.");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config_standard", 0, 1, 0));

HOOK_LISTENER(log);
HOOK_LISTENER(configChanged);

static void updateConfig();

#ifdef WIN32
	static void updateConfigFor(char *configPath, LogType type, int defaultValue);
#else
	static void updateConfigFor(char *configPath, LogType type, char *defaultValue);
#endif

#ifdef WIN32
	static bool inWindowsColorRange(int color);
	static void writeMessage(char *dateTime, int color, char *logType, char *message);
	static void setWindowsConsoleColor(int color);
	static int getWindowsConsoleColor();
#endif

#define COLORS_CONFIG_PATH "kalisko/logColors"
#define ERROR_COLOR_PATH "/error"
#define WARNING_COLOR_PATH "/warning"
#define INFO_COLOR_PATH "/info"
#define DEBUG_COLOR_PATH "/debug"

#ifdef WIN32
	#define STD_ERROR_COLOR 12 // red
	#define STD_WARNING_COLOR 4 // dark red
	#define STD_INFO_COLOR 10 // green
	#define STD_DEBUG_COLOR 9 // blue
#else
	#define STD_ERROR_COLOR "1;31m" // bold red
	#define STD_WARNING_COLOR "31m" // red
	#define STD_INFO_COLOR "32m" // green
	#define STD_DEBUG_COLOR "34m" // blue
#endif

#ifdef WIN32
	int errorColor = STD_ERROR_COLOR;
	int warningColor = STD_WARNING_COLOR;
	int infoColor = STD_INFO_COLOR;
	int debugColor = STD_DEBUG_COLOR;
#else
	char *errorColor = STD_ERROR_COLOR;
	char *warningColor = STD_WARNING_COLOR;
	char *infoColor = STD_INFO_COLOR;
	char *debugColor = STD_DEBUG_COLOR;
#endif

MODULE_INIT
{
	if(!HOOK_ATTACH(log, log)) {
		return false;
	}

	updateConfig(); // we initialize the colors after attaching the log hook so we can see possible problems on the console.

	if(!HOOK_ATTACH(stdConfigChanged, configChanged)) {
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(log, log);
}

/**
 * Log message listener to write them colored into stderr.
 */
HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

	switch(type) {
		case LOG_TYPE_ERROR:
#ifdef WIN32
			writeMessage(dateTime, errorColor, "ERROR", message);
#else
			fprintf(stderr, "%s \033[%sERROR: %s\033[m\n", dateTime, errorColor, message);
#endif

		break;
		case LOG_TYPE_WARNING:
#ifdef WIN32
			writeMessage(dateTime, warningColor, "WARNING", message);
#else
			fprintf(stderr, "%s \033[%sWARNING: %s\033[m\n", dateTime, warningColor, message);
#endif

		break;
		case LOG_TYPE_INFO:
#ifdef WIN32
			writeMessage(dateTime, infoColor, "INFO", message);
#else
			fprintf(stderr, "%s \033[%sINFO: %s\033[m\n", dateTime, infoColor, message);
#endif

		break;
		case LOG_TYPE_DEBUG:
#ifdef WIN32
			writeMessage(dateTime, debugColor, "DEBUG", message);
#else
			fprintf(stderr, "%s \033[%sDEBUG: %s\033[m\n", dateTime, debugColor, message);
#endif
		break;
	}

	free(now);
	free(dateTime);
	fflush(stderr);
}

HOOK_LISTENER(configChanged) {
	updateConfig();
}

/**
 * Reads the standard configs to set the colors as the user want it. If no configuration is set the default colors are used.
 */
static void updateConfig() {
	LOG_INFO("Reading configuration for log_color_console.");

#ifdef WIN32
	updateConfigFor(COLORS_CONFIG_PATH ERROR_COLOR_PATH, LOG_TYPE_ERROR, STD_ERROR_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH WARNING_COLOR_PATH, LOG_TYPE_WARNING, STD_WARNING_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH INFO_COLOR_PATH, LOG_TYPE_INFO, STD_INFO_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH DEBUG_COLOR_PATH, LOG_TYPE_DEBUG, STD_DEBUG_COLOR);
#else
	updateConfigFor(COLORS_CONFIG_PATH ERROR_COLOR_PATH, LOG_TYPE_ERROR, STD_ERROR_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH WARNING_COLOR_PATH, LOG_TYPE_WARNING, STD_WARNING_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH INFO_COLOR_PATH, LOG_TYPE_INFO, STD_INFO_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH DEBUG_COLOR_PATH, LOG_TYPE_DEBUG, STD_DEBUG_COLOR);
#endif
}

/**
 * Reads the standard configs to set the color for a given log type or if nothing is set to default.
 *
 * @param configPath	The configuration path to the color in the configuration file
 * @param type			The LogType for which this setting is to applay
 * @param defaultValue	The default color to use
 */
#ifdef WIN32
	static void updateConfigFor(char *configPath, LogType type, int defaultValue)
#else
	static void updateConfigFor(char *configPath, LogType type, char *defaultValue)
#endif
{
	#ifdef WIN32
		int newColor = defaultValue;
	#else
		char *newColor = defaultValue;
	#endif

	ConfigNodeValue *colorConfig = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)(configPath);
	if(colorConfig) {
		#ifdef WIN32
			if(colorConfig->type == CONFIG_INTEGER) {
				int color = colorConfig->content.integer;

				if(!inWindowsColorRange(color)) {
					LOG_ERROR("On Windows systems the color code must be a number from 0 to 15 (inclusive). Currently it is: %i", color);
				} else {
					newColor = color;
				}
			} else {
				LOG_ERROR("On Windows systems the color code must be a number from 0 to 15 (inclusive).");
			}
	#else
			if(colorConfig->type == CONFIG_STRING) {
				newColor = colorConfig->content.string;
			} else {
				LOG_ERROR("On *nix systems the color code must be a string.");
			}
		#endif
	} else {
		LOG_DEBUG("No color set for log type error. Using default value");
	}

	switch(type) {
		case LOG_TYPE_ERROR:
			errorColor = newColor;
		break;
		case LOG_TYPE_WARNING:
			warningColor = newColor;
		break;
		case LOG_TYPE_INFO:
			infoColor = newColor;
		break;
		case LOG_TYPE_DEBUG:
			debugColor = newColor;
		break;
		default:
			LOG_DEBUG("Unknown LogType value given: '%i'", type);
	}
}

#ifdef WIN32

	/**
	 * Checks if the given color can be used on Windows as a color.
	 *
	 * @param color		The color to check.
	 */
	static bool inWindowsColorRange(int color) {
		return color >= 0 && color <= 15;
	}

	/**
	 * Writes on Windows systems the log message to stderr. This function just makes the source code
	 * more readable.
	 *
	 * @param dateTime	The date & time string
	 * @param color		The color to use on the console
	 * @param logType	The string representing the current log type
	 * @param message	The message itself
	 */
	static void writeMessage(char *dateTime, int color, char *logType, char *message) {
		int currentColor = getWindowsConsoleColor();

		fprintf(stderr, dateTime);
		setWindowsConsoleColor(color);
		fprintf(stderr, " %s: %s\n", logType, message);

		setWindowsConsoleColor(currentColor);
	}

	/**
	 * Sets the foreground color in a Windows console to the given value.
	 *
	 * @param color		The new foreground color
	 */
	static void setWindowsConsoleColor(int color)
	{
		WORD newColor;

		// Get the handle for stderr
		HANDLE hStdOut = GetStdHandle(STD_ERROR_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO bufferInfo;

		if(GetConsoleScreenBufferInfo(hStdOut, &bufferInfo)) {

			// Mask out the foreground color and set it to the given one
			newColor = (bufferInfo.wAttributes & 0xF0) + (color & 0x0F);
			SetConsoleTextAttribute(hStdOut, newColor);
		}
	}

	/**
	 * Returns the current foreground color in a Windows console.
	 *
	 * @return	The current foreground color
	 */
	static int getWindowsConsoleColor()
	{
		// Get the handle for stderr
		HANDLE hStdOut = GetStdHandle(STD_ERROR_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO bufferInfo;

		if(GetConsoleScreenBufferInfo(hStdOut, &bufferInfo)) {
			// Mask out the background color
			return (bufferInfo.wAttributes & 0x0F);
		}

		return 15; // let's say white is the default color
	}
#endif
