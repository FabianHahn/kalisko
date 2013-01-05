/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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


#ifndef EVENT_EVENT_H
#define EVENT_EVENT_H


#include <glib.h>
#include <stdarg.h>
#include <limits.h>
#include "types.h"

/**
 * Generic event listener function pointer type
 */
typedef void (EventListener)(void *subject, const char *event, void *custom_data, va_list args);

/**
 * Predefined set of event listener priorities
 */
typedef enum {
	EVENT_LISTENER_PRIORITY_LOWEST = INT_MIN,
	EVENT_LISTENER_PRIORITY_NORMAL = 0,
	EVENT_LISTENER_PRIORITY_HIGHEST = INT_MAX
} EventListenerPriority;

/**
 * Struct to represent a listener entry
 */
typedef struct
{
	/** The listener function pointer */
	EventListener *listener;
	/** The priority of the listener */
	int priority;
	/** Custom data to pass to the function when triggered */
	void *custom;
} EventListenerEntry;


/**
 * Attaches an event listener to a subject
 *
 * This function is thread-safe
 *
 * @param subject		the subject to attach the event listener to
 * @param event			the event to attach the event listener to
 * @param custom		custom data passed to the listener when the event is triggered
 * @param listener		the listener to attach
 */
API void attachEventListener(void *subject, const char *event, void *custom, EventListener *listener);

/**
 * Attaches an event listener to a subject while allowing to specify a priority
 *
 * The priority parameter specifies the position in the event listener queue. The
 * event listener with the lower priority will be called first, while events attached
 * without priority are classified as EVENT_LISTENER_PRIORITY_NORMAL. Use the
 * EventListenerPriority enum for a predefined set of priorities.
 *
 * This function is thread-safe
 *
 * @param subject		the subject to attach the event listener to
 * @param event			the event to attach the event listener to
 * @param priority		the priority of the event listener
 * @param custom		custom data passed to the listener when the event is triggered
 * @param listener		the listener to attach
 */
API void attachEventListenerWithPriority(void *subject, const char *event, int priority, void *custom, EventListener *listener);

/**
 * Detach an event listener from a subject.
 *
 * This function is thread-safe.
 *
 * @param subject		the subject to detach the event listener from
 * @param event			the event to detach the event listener from
 * @param custom		custom data passed to the listener when the event was triggered
 * @param listener		the listener to attach
 */
API void detachEventListener(void *subject, const char *event, void *custom, EventListener *listener);

/**
 * Triggers an event and notifies all its listeners.
 *
 * This function is both reentrant and thread-safe, meaning you can freely trigger more events while executing an event listener.
 *
 * @param subject		the subject for which the event should be triggered
 * @param event			the event that should be triggered
 * @param ...			the data to pass to the listeners
 * @result				the number of listeners notified, -1 on error
 */
API int triggerEvent(void *subject, const char *event, ...);

/**
 * Returns the listener count for an event on a subject
 *
 * This function is thread-safe.
 *
 * @param subject		the subject to which the event belongs
 * @param event			the vent to lookup the listener count for
 * @result				the number of listeners for that event
 */
API int getEventListenerCount(void *subject, const char *event);

#endif
