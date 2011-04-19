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

#ifndef OPENGL_PRIMITIVE_H
#define OPENGL_PRIMITIVE_H

// Forward declaration
struct OpenGLPrimitiveStruct;

/**
 * Function pointer type to setup OpenGL custom primitives
 */
typedef bool (OpenGLPrimitiveSetupFunction)(struct OpenGLPrimitiveStruct *primitive, const char *model_name, const char *material_name);

/**
 * Function pointer type to draw OpenGL custom primitives
 */
typedef bool (OpenGLPrimitiveDrawFunction)(struct OpenGLPrimitiveStruct *primitive);

/**
 * Function pointer type to free OpenGL custom primitives
 */
typedef bool (OpenGLPrimitiveUpdateFunction)(struct OpenGLPrimitiveStruct *primitive, double dt);

/**
 * Function pointer type to free OpenGL custom primitives
 */
typedef void (OpenGLPrimitiveFreeFunction)(struct OpenGLPrimitiveStruct *primitive);

/**
 * OpenGL primitive data type
 */
struct OpenGLPrimitiveStruct {
	/** An optional type string to identify the type of the primitive */
	char *type;
	/** The data of the primitive */
	void *data;
	/** The init function of the primitive */
	OpenGLPrimitiveSetupFunction *setup_function;
	/** The draw function of the primitive */
	OpenGLPrimitiveDrawFunction *draw_function;
	/** The update function of the primitive */
	OpenGLPrimitiveUpdateFunction *update_function;
	/** The free function of the primitive */
	OpenGLPrimitiveFreeFunction *free_function;
};

typedef struct OpenGLPrimitiveStruct OpenGLPrimitive;

/**
 * Sets up an OpenGL primitive for a model
 *
 * @param primitive			the primitive to setup
 * @param model_name		the model name to setup the primitive for
 * @param mesh_name			the mesh name to setup the primitive for
 * @result					true if successful
 */
static inline bool setupOpenGLPrimitive(OpenGLPrimitive *primitive, const char *model_name, const char *mesh_name)
{
	if(primitive->setup_function != NULL) {
		return primitive->setup_function(primitive, model_name, mesh_name);
	} else {
		return true;
	}
}

/**
 * Draws an OpenGL primitive
 *
 * @param primitive			the primitive to draw
 * @result					true if successful
 */
static inline bool drawOpenGLPrimitive(OpenGLPrimitive *primitive)
{
	if(primitive->draw_function != NULL) {
		return primitive->draw_function(primitive);
	} else {
		return true;
	}
}

/**
 * Updates an OpenGL primitive
 *
 * @param primitive			the primitive to update
 * @result					true if successful
 */
static inline bool updateOpenGLPrimitive(OpenGLPrimitive *primitive, double dt)
{
	if(primitive->update_function != NULL) {
		return primitive->update_function(primitive, dt);
	} else {
		return true;
	}
}

/**
 * Frees an OpenGL primitive
 *
 * @param primitive			the primitive to draw
 */
static inline void freeOpenGLPrimitive(OpenGLPrimitive *primitive)
{
	primitive->free_function(primitive);
}

#endif
