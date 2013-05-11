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


#include <stdlib.h>
#include <stdio.h> // fprintf
#include <glib.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "dll.h"
#include "module.h"
#include "modules/config/config.h"
#include "modules/event/event.h"

#include "log.h"
#define API

MODULE_NAME("log_color_console");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Kalisko console log provider with colored output.");
MODULE_VERSION(0, 3, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config", 0, 3, 8), MODULE_DEPENDENCY("event", 0, 1, 2), MODULE_DEPENDENCY("log_event", 0, 1, 1));

#ifdef WIN32
	typedef int ColorCode;
#else
	typedef char* ColorCode;
#endif

static void listener_log(void *subject, const char *event, void *data, va_list args);
static void listener_reloadedConfig(void *subject, const char *event, void *data, va_list args);

static void updateConfig();
static void updateConfigFor(char *configPath, LogLevel level, ColorCode defaultValue);
static ColorCode getLogLevelColorCode(LogLevel level);

#ifdef WIN32
	static bool inWindowsColorRange(ColorCode color);
	static void writeMessage(GString *dateTime, ColorCode color, const char *module, char *logType, char *message);
	static void setWindowsConsoleColor(ColorCode color);
	static ColorCode getWindowsConsoleColor();
#endif

#define COLORS_CONFIG_PATH "logColors"
#define ERROR_COLOR_PATH "/error"
#define WARNING_COLOR_PATH "/warning"
#define NOTICE_COLOR_PATH "/notice"
#define INFO_COLOR_PATH "/info"
#define TRACE_COLOR_PATH "/trace"

#ifdef WIN32
	#define STD_ERROR_COLOR 12 // red
	#define STD_WARNING_COLOR 14 // yellow
	#define STD_NOTICE_COLOR 10 // lime
	#define STD_INFO_COLOR 11 // aqua
	#define STD_TRACE_COLOR 4 // maroon
#else
	#define STD_ERROR_COLOR "31m" // bold red
	#define STD_WARNING_COLOR "33m" // yellow
	#define STD_NOTICE_COLOR "32m" // green
	#define STD_INFO_COLOR "34m" // blue
	#define STD_TRACE_COLOR "36m" // cyan
#endif

static ColorCode errorColor = STD_ERROR_COLOR;
static ColorCode warningColor = STD_WARNING_COLOR;
static ColorCode noticeColor = STD_NOTICE_COLOR;
static ColorCode infoColor = STD_INFO_COLOR;
static ColorCode traceColor = STD_TRACE_COLOR;

MODULE_INIT
{
	$(void, event, attachEventListener)(NULL, "log", NULL, &listener_log);
	$(void, event, attachEventListener)(NULL, "reloadedConfig", NULL, &listener_reloadedConfig);

	updateConfig(); // we initialize the colors after attaching the log hook so we can see possible problems on the console.

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(NULL, "log", NULL, &listener_log);
	$(void, event, detachEventListener)(NULL, "reloadedConfig", NULL, &listener_reloadedConfig);
}

/**
 * Log message listener to write them colored into stderr.
 */
static void listener_log(void *subject, const char *event, void *data, va_list args)
{
	const char *module = va_arg(args, const char *);
	LogLevel level = va_arg(args, LogLevel);
	char *message = va_arg(args, char *);

	GDateTime *now = g_date_time_new_now_local();
	unsigned int hour = g_date_time_get_hour(now);
	unsigned int minute = g_date_time_get_minute(now);
	unsigned int second = g_date_time_get_second(now);
    g_date_time_unref(now);

#ifdef WIN32
    GString *dateTime = g_string_new("");
    g_string_append_printf(dateTime, "[%02u:%02u:%02u]", hour, minute, second);
    writeMessage(dateTime, getLogLevelColorCode(level), module, getStaticLogLevelName(level), message);
    g_string_free(dateTime, true);
#else
    fprintf(stderr, "[%02u:%02u:%02u] \033[%s[%s:%s] %s\033[m\n", hour, minute, second, getLogLevelColorCode(level), module, getStaticLogLevelName(level), message);
#endif

	fflush(stderr);
}

static void listener_reloadedConfig(void *subject, const char *event, void *data, va_list args)
{
	updateConfig();
}

/**
 * Reads the standard configs to set the colors as the user want it. If no configuration is set the default colors are used.
 */
static void updateConfig() {
	logNotice("Reading configuration for log_color_console.");
	updateConfigFor(COLORS_CONFIG_PATH ERROR_COLOR_PATH, LOG_LEVEL_ERROR, STD_ERROR_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH WARNING_COLOR_PATH, LOG_LEVEL_WARNING, STD_WARNING_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH NOTICE_COLOR_PATH, LOG_LEVEL_NOTICE, STD_NOTICE_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH INFO_COLOR_PATH, LOG_LEVEL_INFO, STD_INFO_COLOR);
	updateConfigFor(COLORS_CONFIG_PATH TRACE_COLOR_PATH, LOG_LEVEL_TRACE, STD_TRACE_COLOR);
}

/**
 * Reads the standard configs to set the color for a given log type or if nothing is set to default.
 *
 * @param configPath	The configuration path to the color in the configuration file
 * @param type			The LogType for which this setting is to applay
 * @param defaultValue	The default color to use
 */
static void updateConfigFor(char *configPath, LogLevel level, ColorCode defaultValue)
{
	ColorCode newColor = defaultValue;
	Store *colorConfig = $(Store *, config, getConfigPath)(configPath);
	if(colorConfig) {
#ifdef WIN32
		if(colorConfig->type == STORE_INTEGER) {
			int color = colorConfig->content.integer;
			if(!inWindowsColorRange(color)) {
				logError("On Windows systems the color code must be a number from 0 to 15 (inclusive). Currently it is: %i", color);
			} else {
				newColor = color;
			}
		} else {
			logError("On Windows systems the color code must be a number from 0 to 15 (inclusive).");
		}
#else
		if(colorConfig->type == STORE_STRING) {
			newColor = colorConfig->content.string;
		} else {
			logError("On *nix systems the color code must be a string.");
		}
#endif
	} else {
		logInfo("No color set for log type error. Using default value");
	}

	switch(level) {
		case LOG_LEVEL_ERROR:
			errorColor = newColor;
		break;
		case LOG_LEVEL_WARNING:
			warningColor = newColor;
		break;
		case LOG_LEVEL_NOTICE:
			infoColor = newColor;
		break;
		case LOG_LEVEL_INFO:
			infoColor = newColor;
		break;
		case LOG_LEVEL_TRACE:
		default:
			traceColor = newColor;
		break;
	}
}

static ColorCode getLogLevelColorCode(LogLevel level)
{
	switch(level) {
		case LOG_LEVEL_ERROR:
			return errorColor;
		case LOG_LEVEL_WARNING:
			return warningColor;
		case LOG_LEVEL_NOTICE:
			return noticeColor;
		case LOG_LEVEL_INFO:
			return infoColor;
		case LOG_LEVEL_TRACE:
		default:
			return traceColor;
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
	 * @param module	the module in which the log message occured
	 * @param logType	The string representing the current log type
	 * @param message	The message itself
	 */
	static void writeMessage(GString *dateTime, int color, const char *module, char *logType, char *message) {
		int currentColor = getWindowsConsoleColor();

		fprintf(stderr, "%s", dateTime->str);
		setWindowsConsoleColor(color);
		fprintf(stderr, " [%s:%s] %s\n", module, logType, message);

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
