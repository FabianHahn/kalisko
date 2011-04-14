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

#ifndef PARTICLE_PARTICLE_H
#define PARTICLE_PARTICLE_H

#include <GL/glew.h>
#include "modules/opengl/primitive.h"

/**
 * Struct representing a particle vertex
 */
typedef struct {
	/** The position of the particle vertex */
	float position[3];
	/** The corner of the particle vertex in its sprite */
	float corner[2];
	/** The velocity of the particle */
	float velocity[3];
	/** The time in seconds that the particle is already alive */
	float time;
} ParticleVertex;

/**
 * Struct representing a particle sprite
 */
typedef struct {
	/** The indices of the particle sprite */
	unsigned int indices[6];
} ParticleSprite;

/**
 * Struct representing an OpenGL particle effect
 */
typedef struct {
	/** The vertices to render */
	ParticleVertex *vertices;
	/** The sprites to render */
	ParticleSprite *sprites;
	/** The lifetime of a particle in seconds */
	float lifetime;
	/** The number of particles in the particle effect */
	unsigned int num_particles;
	/** The OpenGL vertex buffer associated with this particle effect */
	GLuint vertexBuffer;
	/** The OpenGL index buffer associated with this particle effect */
	GLuint indexBuffer;
	/** The OpenGL primitive used to render the particle effect */
	OpenGLPrimitive primitive;
} OpenGLParticles;

API OpenGLPrimitive *createOpenGLPrimitiveParticles(unsigned int num_particles, double lifetime);
API bool initOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);
API OpenGLParticles *getOpenGLParticles(OpenGLPrimitive *primitive);
API bool simulateOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, double dt);
API bool updateOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);
API bool drawOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);
API void freeOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);

#endif
