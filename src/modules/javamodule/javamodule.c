/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2014, Kalisko Project Leaders
 * Copyright (c) 2014, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *		 @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *		 @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *			 in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jni.h>

#include "dll.h"
#define API
#include "modules/javamodule/javamodule.h"

MODULE_NAME("javamodule");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module runs a jvm and adds support for executing modules written in Java.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_NODEPS;

#define CLASSPATH_OPTION "-Djava.class.path=/home/dinowernli/playground/jni/java"

static JavaVM *java_vm;
static JNIEnv *java_env;

MODULE_INIT
{
	JavaVMInitArgs vm_args;
	vm_args.version = JNI_VERSION_1_6;
	vm_args.ignoreUnrecognized = 1;

	JavaVMOption options[1];
	options[0].optionString = CLASSPATH_OPTION;
	vm_args.options = options;
	vm_args.nOptions = 1;

	jint result = JNI_CreateJavaVM(&java_vm, (void **)&java_env, &vm_args);
	if (result < 0) {
		logError("Unable to create JVM");
		return false;
  }
	return true;
}

MODULE_FINALIZE
{
	// Teardown the JVM.
}

API bool executeJavaModule(char *moduleClass)
{
	/*
	jclass cls = (*env)->FindClass(env, moduleClass);
 	if (cls == NULL) {
		logError("Could not find module class %s", moduleClass);
		return false;
 	}

 	jmethodID mid = (*env)->GetStaticMethodID(env, cls, "main", "([Ljava/lang/String;)V");
 	if (mid == NULL) {
		logError("Could not find module class %s", moduleClass);
		return false;
 	}

	jstring jstr;
	jstring argString = (*env)->NewStringUTF(env, "");
	jobjectArray args = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/String"), jstr);
	if (args == NULL) {
		printf("Out of memory\n");
		return 1;
	}
	*/
	logWarning("Executing Java modules not yet implemented");
	return false;
}
