#include <stdio.h> // fprintf
#include <time.h> //time_t, strftime
#include <glib.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "api.h"

#define TIME_STRING_BUFFER_SIZE 18

HOOK_LISTENER(log);

API bool module_init()
{
	return HOOK_ATTACH(log, log);
}

API void module_finalize()
{
	HOOK_DETACH(log, log);
}

API GList *module_depends()
{
	return NULL;
}

/**
 * Log message listener to write them into stdout.
 */
HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	char timeString[TIME_STRING_BUFFER_SIZE] = "[unknown time]"; // pay attention that the string is not longer than 'TIME_STRING_BUFFER_SIZE'
	time_t currentTime = time(NULL);
	if(currentTime != -1)
	{
		struct tm *timeInfo = localtime(&currentTime);
		if(localtime != NULL)
		{
			strftime(timeString, TIME_STRING_BUFFER_SIZE, "%x %X", timeInfo);
		}
	}

	switch(type) {
		case LOG_ERROR:
			fprintf(stdout, "%s ERROR: %s\n", timeString, message);
		break;
		case LOG_WARNING:
			fprintf(stdout, "%s WARNING: %s\n", timeString, message);
		break;
		case LOG_INFO:
			fprintf(stdout, "%s INFO: %s\n", timeString, message);
		break;
		case LOG_DEBUG:
			fprintf(stdout, "%s DEBUG: %s\n", timeString, message);
		break;
	}

	fflush(stdout);
}
