#include <stdio.h>
#include <glib.h>

// The first include after system headers must always be dll.h
#include "dll.h"
// Next, headers from other modules or the core follow
#include "hooks.h"
#include "log.h"

// api.h must be included before own headers and definitions that could be called from other modules
#include "api.h"
// Include our own header only after api.h
#include "sample.h"

// Every function that could be called from another module must have an API marker

// module_ functions also need API
API bool module_init()
{
	logInfo("This is a log message from the sample module. Hi there!");
	printf("Hello world\n");
	HOOK_ADD(sample);

	return true;
}

API void module_finalize()
{
	HOOK_DEL(sample);
}

API GList *module_depends()
{
	return NULL;
}

// This is a function that's exported to the global scope, hence it needs the API marker
API void foo()
{

}
