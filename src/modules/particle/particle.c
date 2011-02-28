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

#include <math.h>
#include <assert.h>
#include <glib.h>
#include <stdlib.h>
#include <GL/glew.h>
#include "dll.h"
#include "modules/opengl/primitive.h"
#include "modules/opengl/shader.h"
#include "modules/opengl/opengl.h"
#include "api.h"
#include "particle.h"

MODULE_NAME("particle");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL particle effects");
MODULE_VERSION(0, 2, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("opengl", 0, 16, 0));

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Returns a random float number between 0 and 1
 *
 * @result			the generated random number
 */
static float frand()
{
	return (float) rand() / RAND_MAX;
}

static void initParticle(OpenGLParticles *particles, unsigned int i);

/**
 * Creates a new OpenGL particle effect primitive
 *
 * @param num_particles		the number of particles used for the effect
 * @param lifetime			the lifetime of a particle in the particle effect
 * @result					the created OpenGL particle effect primitive object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveParticles(unsigned int num_particles, double lifetime)
{
	OpenGLParticles *particles = ALLOCATE_OBJECT(OpenGLParticles);
	particles->particles = ALLOCATE_OBJECTS(Particle, num_particles);
	particles->properties = ALLOCATE_OBJECTS(ParticleProperty, num_particles);
	particles->num_particles = num_particles;
	particles->lifetime = lifetime;
	particles->primitive.type = "particles";
	particles->primitive.data = particles;
	particles->primitive.draw_function = &drawOpenGLPrimitiveParticles;
	particles->primitive.free_function = &freeOpenGLPrimitiveParticles;

	glGenBuffers(1, &particles->vertexBuffer);
	initOpenGLPrimitiveParticles(&particles->primitive);
	updateOpenGLPrimitiveParticles(&particles->primitive);

	if($(bool, opengl, checkOpenGLError)()) {
		freeOpenGLPrimitiveParticles(&particles->primitive);
		return NULL;
	}

	return &particles->primitive;
}

/**
 * Initlaizes an OpenGL particle effect primitive
 *
 * @param primitive			the OpenGL particle effect primitive to initialize
 * @result					true if successful
 */
API bool initOpenGLPrimitiveParticles(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to initialize OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;

	for(unsigned int i = 0; i < particles->num_particles; i++) {
		initParticle(particles, i);
		particles->properties[i].alive = frand() * particles->lifetime;
	}

	return true;
}

/**
 * Returns the associated OpenGLParticles object for an OpenGL particle effect primitive
 *
 * @param primitive			the OpenGL particle effect primitive for which the OpenGLParticles object should be retrieved
 * @result					the OpenGLParticles object or NULL if the primitive is not an OpenGL particle effect primitive
 */
API OpenGLParticles *getOpenGLParticles(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to retrieve OpenGL particles: Primitive is not a particle effect");
		return NULL;
	}

	return primitive->data;
}

/**
 * Simulates an OpenGL particle effect primitive by moving forward a specified timestep
 *
 * @param primitive			the particle effect primitive to simulate
 * @param dt				the timestep in microseconds to move forward
 * @result					true if successful
 */
API bool simulateOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, double dt)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to simulate OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;

	for(unsigned int i = 0; i < particles->num_particles; i++) {
		if((particles->properties[i].alive += dt) > particles->lifetime) {
			initParticle(particles, i);
			continue;
		}

		particles->properties[i].acceleration[0] = 100.0f * dt * (frand() - 0.5f);
		particles->properties[i].acceleration[2] = 100.0f * dt * (frand() - 0.5f);

		particles->properties[i].velocity[0] += particles->properties[i].acceleration[0] * dt;
		particles->properties[i].velocity[1] += particles->properties[i].acceleration[1] * dt;
		particles->properties[i].velocity[2] += particles->properties[i].acceleration[2] * dt;
		particles->particles[i].position[0] += particles->properties[i].velocity[0] * dt;
		particles->particles[i].position[1] += particles->properties[i].velocity[1] * dt;
		particles->particles[i].position[2] += particles->properties[i].velocity[2] * dt;
	}

	return true;
}

/**
 * Updates a particle effect primitive by synchronizing it with its associated OpenGL buffer objects
 *
 * @param primitive			the particle effect primitive to be updated
 * @result					true if successful
 */
API bool updateOpenGLPrimitiveParticles(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to update OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;

	glBindBuffer(GL_ARRAY_BUFFER, particles->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles->num_particles, particles->particles, GL_DYNAMIC_DRAW);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Draws an OpenGL particle effect primitive
 *
 * @param primitive			the particle effect primitive to draw
 */
API bool drawOpenGLPrimitiveParticles(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to draw OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;

	glBindBuffer(GL_ARRAY_BUFFER, particles->vertexBuffer);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_VERTEX, 3, GL_FLOAT, false, sizeof(Particle), NULL + offsetof(Particle, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_VERTEX);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_COLOR, 4, GL_FLOAT, false, sizeof(Particle), NULL + offsetof(Particle, color));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_COLOR);

	if(checkOpenGLError()) {
		return false;
	}

	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glDrawArrays(GL_POINTS, 0, particles->num_particles);

	if(checkOpenGLError()) {
		return false;
	}

	return true;
}

/**
 * Frees an OpenGL particle effect primitive
 *
 * @param primitive			the particle effect primitive to free
 */
API void freeOpenGLPrimitiveParticles(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to free OpenGL particles: Primitive is not a particle effect");
		return;
	}

	OpenGLParticles *particles = primitive->data;

	glDeleteBuffers(1, &particles->vertexBuffer);
	free(particles->particles);
	free(particles);
}

/**
 * Initializes a particle
 *
 * @param particles			the particle effect in which to initialize a particle
 * @param i					the number of the particle to initialize
 */
static void initParticle(OpenGLParticles *particles, unsigned int i)
{
	particles->particles[i].position[0] = frand() - 0.5f;
	particles->particles[i].position[1] = 0.0f;
	particles->particles[i].position[2] = frand() - 0.5f;
	particles->particles[i].color[0] = 1.0f;
	particles->particles[i].color[1] = frand();
	particles->particles[i].color[2] = frand();
	particles->particles[i].color[3] = 1.0f;
	particles->properties[i].acceleration[0] = 0.0f;
	particles->properties[i].acceleration[1] = 0.0f;
	particles->properties[i].acceleration[2] = 0.0f;
	particles->properties[i].velocity[0] = 0.0f;
	particles->properties[i].velocity[1] = 0.5f;
	particles->properties[i].velocity[2] = 0.0f;
	particles->properties[i].alive = 0.0f;
}
