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
 * Function pointer type to draw OpenGL custom primitives
 */
typedef bool (OpenGLPrimitiveDrawFunction)(struct OpenGLPrimitiveStruct *primitive);

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
	/** The draw function of the primitive */
	OpenGLPrimitiveDrawFunction *draw_function;
	/** The free function of the primitive */
	OpenGLPrimitiveFreeFunction *free_function;
};

typedef struct OpenGLPrimitiveStruct OpenGLPrimitive;

/**
 * Draws an OpenGL primitive
 *
 * @param primitive			the primitive to draw
 */
static inline void drawOpenGLPrimitive(OpenGLPrimitive *primitive)
{
	primitive->draw_function(primitive);
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