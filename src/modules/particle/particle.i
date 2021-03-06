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
#include "modules/opengl/model.h"
#include "modules/linalg/Vector.h"

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
	/** The birth time of the particle */
	float birth;
	/** The angular velocity of the particle */
	float angularVelocity;
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
	/** The current particle time */
	float time;
	/** The number of particles in the particle effect */
	unsigned int num_particles;
	/** The OpenGL vertex buffer associated with this particle effect */
	GLuint vertexBuffer;
	/** The OpenGL index buffer associated with this particle effect */
	GLuint indexBuffer;
	/** The OpenGL primitive used to render the particle effect */
	OpenGLPrimitive primitive;
	/** The configurable properties of the particle effect */
	struct {
		/** The lifetime of a particle in seconds */
		float lifetime;
		/** The mean position of a new particle */
		Vector *positionMean;
		/** The standard deviation of a new particle's position */
		Vector *positionStd;
		/** The mean velocity of a new particle */
		Vector *velocityMean;
		/** The standard deviation of a new particle's velocity */
		Vector *velocityStd;
		/** The start size of a particle */
		float startSize;
		/** The end size of a particle */
		float endSize;
		/** The aspect ratio of a particle */
		float aspectRatio;
		/** The mean of a new particle's angular velocity */
		float angularVelocityMean;
		/** The standard deviation of a new particle's angular velocity */
		float angularVelocityStd;
	} properties;
} OpenGLParticles;


/**
 * Creates a new OpenGL particle effect primitive
 *
 * @param num_particles		the number of particles used for the effect
 * @result					the created OpenGL particle effect primitive object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveParticles(unsigned int num_particles);

/**
 * Initlaizes an OpenGL particle effect primitive
 *
 * @param primitive			the OpenGL particle effect primitive to initialize
 * @result					true if successful
 */
API bool initOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);

/**
 * Sets up an OpenGL particle effect primitive for a model
 *
 * @param primitive			the particle effect primitive to setup
 * @param model				the model to setup the particle effect primitive for
 * @param material			the material name to setup the particle effect primitive for
 * @result					true if successful
 */
API bool setupOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, OpenGLModel *model, const char *material);

/**
 * Returns the associated OpenGLParticles object for an OpenGL particle effect primitive
 *
 * @param primitive			the OpenGL particle effect primitive for which the OpenGLParticles object should be retrieved
 * @result					the OpenGLParticles object or NULL if the primitive is not an OpenGL particle effect primitive
 */
API OpenGLParticles *getOpenGLParticles(OpenGLPrimitive *primitive);

/**
 * Updates an OpenGL particle effect primitive by moving forward a specified timestep
 *
 * @param primitive			the particle effect primitive to update
 * @param dt				the timestep in seconds to move forward
 * @result					true if successful
 */
API bool updateOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, double dt);

/**
 * Synchronizes a particle effect primitive with its associated OpenGL buffer objects
 *
 * @param primitive			the particle effect primitive to be synchronized
 * @result					true if successful
 */
API bool synchronizeOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);

/**
 * Draws an OpenGL particle effect primitive
 *
 * @param primitive			the particle effect primitive to draw
 * @param options_p			a pointer to custom options to be considered for this draw call
 */
API bool drawOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, void *options_p);

/**
 * Frees an OpenGL particle effect primitive
 *
 * @param primitive			the particle effect primitive to free
 */
API void freeOpenGLPrimitiveParticles(OpenGLPrimitive *primitive);

#endif
