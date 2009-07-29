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

#include <Python.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "memory_alloc.h"

#include "api.h"
#include "lang_python.h"


API bool module_init()
{
	Py_Initialize();
	PyEval_InitThreads();
	return true;
}

API void module_finalize()
{
	HOOK_DEL(python_run_code);

	if(Py_IsInitialized())
	{
		Py_Finalize();
	}
}

API GList *module_depends()
{
	return NULL;
}

API void python_interpreter_init(PythonInterpreter *interpreter)
{
	LOG_INFO("Init new PythonInterpreter");
	interpreter->threadState = Py_NewInterpreter();
}

API void python_run_code(PythonInterpreter *interpreter, char *code)
{
	LOG_DEBUG("Run Python-Code: %s", code);
	PyThreadState_Swap(interpreter->threadState);
	PyRun_SimpleString(code);
}

