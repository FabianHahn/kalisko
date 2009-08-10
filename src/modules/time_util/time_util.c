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

#include <time.h>
#include <string.h>

#include "dll.h"
#include "module.h"
#include "memory_alloc.h"

#include "api.h"
#include "modules/time_util/time_util.h"

MODULE_NAME("time_util");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Utility module that provides frequently used date and time functions");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Returns a string representing the current date and time
 *
 * @return The current date and time as a string. This string must be freed
 */
API char *getCurrentDateTimeString()
{
	// pay attention that the string is not longer than 'TIME_STRING_BUFFER_SIZE'
	char timeString[TIME_STRING_BUFFER_SIZE] = "[unknown time]";

	time_t currentTime = time(NULL);
	if(currentTime != -1) {
		struct tm *timeInfo = localtime(&currentTime);
		if(localtime != NULL) {
			strftime(timeString, TIME_STRING_BUFFER_SIZE, "%x %X", timeInfo);
		}
	}

	return strdup(timeString);
}
