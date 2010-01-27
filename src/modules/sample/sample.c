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

// First include all used system headers (those NOT in the project folder)
#include <stdio.h> // we're going to use printf later, so make sure it's declared
#include <glib.h> // we don't really use glib in this module, but since it's used in almost every serious module, we include it here to show you where its include line should be placed :)

// The first include after system headers must always be dll.h
#include "dll.h"
// Next, headers from other modules or the core follow
#include "hooks.h"
// The following three headers declare the core macros we're going to use as an example below
#include "log.h"
#include "types.h"
#include "timer.h"

// api.h must be included before own headers and definitions that could be called from other modules
#include "api.h"
// Include our own header only after api.h
#include "sample.h"

// Ok, everything included, let's define all the module's metadata
MODULE_NAME("sample"); // this name must be equal to the module's folder and the library name (without prefix) in the SConscript makefile
MODULE_AUTHOR("The Kalisko team"); // specifies who wrote the module
MODULE_DESCRIPTION("This is a sample module intended to show new module developers how the Kalisko module system works."); // short description of what the module does
MODULE_VERSION(0, 1, 0); // this module's major, minor and patch version (integer numbers), a source version from svn/hg will be appended automatically as fourth version component
MODULE_BCVERSION(0, 1, 0); // the version this module is backwards compatible to, i.e. the oldest version other modules can still depend on to use this module
MODULE_NODEPS; // module dependencies of this module, this one has none at all; to specify a module dependency, use the MODULE_DEPENDS macro and specify several MODULE_DEPENDENCY entries as arguments (see the sample test suite for an example)

TIMER_CALLBACK(sample); // forward declaration of the timer callback we're going to define later
static GTimeVal *timeout; // an identifier for our scheduled timeout callback

// The actual code of the module starts here...

MODULE_INIT // This function is called upon initialization of the module and can perform further startup work. Modules can expect all dependency modules to be already loaded at this point. The function must return true if the initialization is successful - if it returns false, the module is immediately unloaded.
{
	LOG_INFO("This is a log message from the sample module. Hi there!"); // This macro broadcasts an info log message to all registered log providers
	HOOK_ADD(sample); // Adds a sample hook that can be triggered to notify all its possible listeners
	timeout = TIMER_ADD_TIMEOUT(10 * G_USEC_PER_SEC, sample); // schedules the sample callback to be executed after 10 seconds

	return true; // initialization successful
}

MODULE_FINALIZE // This function is called before the module is finally unloaded, so all cleanup work must be done here. Be sure to unregister
{
	if(timeout != NULL) { // check if the timeout needs to be unregistered
		TIMER_DEL(timeout); // unregister the timeout if it was not yet called, so nothing evil will happen
	}

	HOOK_DEL(sample); // also remove the hook we created before
}

TIMER_CALLBACK(sample) // sample callback function
{
	printf("Hello world from timer!\n"); // print hello world to standard output ;)
	timeout = NULL; // make sure we don't free the timeout in finalization since it's gone now
}

// Every function that could be called from another module must have an API marker
// This is a function that's exported to the global scope, hence it needs the API marker
API int add(int a, int b)
{
	return a + b; // just add two ints and return
}
