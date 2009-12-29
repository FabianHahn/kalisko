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

#include "log.h"
#include "api.h"

MODULE_NAME("log_color_console");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Kalisko console log provider with colored output.");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

HOOK_LISTENER(log);

#ifdef WIN32
	static void writeMessage(char *dateTime, int color, char *logType, char *message);
	static void setWindowsConsoleColor(int color);
	static int getWindowsConsoleColor();
#endif

MODULE_INIT
{
	return HOOK_ATTACH(log, log);
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
			writeMessage(dateTime, 12, "ERROR", message); // 12 -> red
#else
			fprintf(stderr, "%s \033[1;31mERROR: %s\033[m\n", dateTime, message); // bold red
#endif

		break;
		case LOG_TYPE_WARNING:
#ifdef WIN32
			writeMessage(dateTime, 4, "WARNING", message); // 4 -> dark red
#else
			fprintf(stderr, "%s \033[31mWARNING: %s\033[m\n", dateTime, message); // red
#endif

		break;
		case LOG_TYPE_INFO:
#ifdef WIN32
			writeMessage(dateTime, 10, "INFO", message); // 10 -> green
#else
			fprintf(stderr, "%s \033[32mINFO: %s\033[m\n", dateTime, message); // green
#endif

		break;
		case LOG_TYPE_DEBUG:
#ifdef WIN32
			writeMessage(dateTime, 9, "DEBUG", message); // 9 -> blue
#else
			fprintf(stderr, "%s \033[34mDEBUG: %s\033[m\n", dateTime, message); // blue
#endif
		break;
	}

	free(now);
	free(dateTime);
	fflush(stderr);
}

#ifdef WIN32
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
		setWindowsConsoleColor(color); // red
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
