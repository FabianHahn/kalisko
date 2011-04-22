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
#include "modules/scene/primitive.h"
#include "modules/opengl/primitive.h"
#include "modules/opengl/shader.h"
#include "modules/opengl/opengl.h"
#include "modules/opengl/material.h"
#include "modules/random/random.h"
#include "modules/linalg/Vector.h"
#include "api.h"
#include "particle.h"
#include "scene.h"

MODULE_NAME("particle");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module for OpenGL particle effects");
MODULE_VERSION(0, 6, 5);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 11), MODULE_DEPENDENCY("scene", 0, 4, 4), MODULE_DEPENDENCY("opengl", 0, 19, 1), MODULE_DEPENDENCY("random", 0, 2, 0), MODULE_DEPENDENCY("linalg", 0, 3, 3));

MODULE_INIT
{
	return $(bool, scene, registerOpenGLPrimitiveSceneParser)("particles", &parseOpenGLScenePrimitiveParticles);
}

MODULE_FINALIZE
{
	$(bool, scene, unregisterOpenGLPrimitiveSceneParser)("particles");
}

static void initParticle(OpenGLParticles *particles, unsigned int i);

/**
 * Creates a new OpenGL particle effect primitive
 *
 * @param num_particles		the number of particles used for the effect
 * @result					the created OpenGL particle effect primitive object or NULL on failure
 */
API OpenGLPrimitive *createOpenGLPrimitiveParticles(unsigned int num_particles)
{
	OpenGLParticles *particles = ALLOCATE_OBJECT(OpenGLParticles);
	particles->vertices = ALLOCATE_OBJECTS(ParticleVertex, num_particles * 4);
	particles->sprites = ALLOCATE_OBJECTS(ParticleSprite, num_particles);
	particles->num_particles = num_particles;
	particles->time = 0.0f;
	particles->primitive.type = "particles";
	particles->primitive.data = particles;
	particles->primitive.setup_function = &setupOpenGLPrimitiveParticles;
	particles->primitive.draw_function = &drawOpenGLPrimitiveParticles;
	particles->primitive.update_function = &updateOpenGLPrimitiveParticles;
	particles->primitive.free_function = &freeOpenGLPrimitiveParticles;
	particles->properties.lifetime = 3.0f;
	particles->properties.positionMean = $(Vector *, linalg, createVector3)(0.0f, 0.0f, 0.0f);
	particles->properties.positionStd = $(Vector *, linalg, createVector3)(1.0f, 0.0f, 1.0f);
	particles->properties.velocityMean = $(Vector *, linalg, createVector3)(0.0f, 1.0f, 0.0f);
	particles->properties.velocityStd = $(Vector *, linalg, createVector3)(0.0f, 0.0f, 0.0f);
	particles->properties.startSize = 0.1f;
	particles->properties.endSize = 0.2f;
	particles->properties.aspectRatio = 1.0f;

	glGenBuffers(1, &particles->vertexBuffer);
	glGenBuffers(1, &particles->indexBuffer);
	initOpenGLPrimitiveParticles(&particles->primitive);
	synchronizeOpenGLPrimitiveParticles(&particles->primitive);

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
		float birth = -randomUniform() * particles->properties.lifetime;
		particles->vertices[4*i+0].birth = birth;
		particles->vertices[4*i+1].birth = birth;
		particles->vertices[4*i+2].birth = birth;
		particles->vertices[4*i+3].birth = birth;
		particles->sprites[i].indices[0] = 4*i+2;
		particles->sprites[i].indices[1] = 4*i+1;
		particles->sprites[i].indices[2] = 4*i+0;
		particles->sprites[i].indices[3] = 4*i+1;
		particles->sprites[i].indices[4] = 4*i+2;
		particles->sprites[i].indices[5] = 4*i+3;
	}

	return true;
}

/**
 * Sets up an OpenGL particle effect primitive for a model
 *
 * @param primitive			the particle effect primitive to setup
 * @param model_name		the model name to setup the particle effect primitive for
 * @param mesh_name			the mesh name to setup the particle effect primitive for
 * @result					true if successful
 */
API bool setupOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, const char *model_name, const char *material_name)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to initialize OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;

	$(bool, opengl, detachOpenGLMaterialUniform)(material_name, "time");
	OpenGLUniform *timeUniform = $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&particles->time);
	$(bool, opengl, attachOpenGLMaterialUniform)(material_name, "time", timeUniform);

	$(bool, opengl, detachOpenGLMaterialUniform)(material_name, "lifetime");
	OpenGLUniform *lifetimeUniform = $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&particles->properties.lifetime);
	$(bool, opengl, attachOpenGLMaterialUniform)(material_name, "lifetime", lifetimeUniform);

	$(bool, opengl, detachOpenGLMaterialUniform)(material_name, "startSize");
	OpenGLUniform *startSizeUniform = $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&particles->properties.startSize);
	$(bool, opengl, attachOpenGLMaterialUniform)(material_name, "startSize", startSizeUniform);

	$(bool, opengl, detachOpenGLMaterialUniform)(material_name, "endSize");
	OpenGLUniform *endSizeUniform = $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&particles->properties.endSize);
	$(bool, opengl, attachOpenGLMaterialUniform)(material_name, "endSize", endSizeUniform);

	$(bool, opengl, detachOpenGLMaterialUniform)(material_name, "aspectRatio");
	OpenGLUniform *aspectRatioUniform = $(OpenGLUniform *, opengl, createOpenGLUniformFloatPointer)(&particles->properties.aspectRatio);
	$(bool, opengl, attachOpenGLMaterialUniform)(material_name, "aspectRatio", aspectRatioUniform);

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
 * Updates an OpenGL particle effect primitive by moving forward a specified timestep
 *
 * @param primitive			the particle effect primitive to update
 * @param dt				the timestep in seconds to move forward
 * @result					true if successful
 */
API bool updateOpenGLPrimitiveParticles(OpenGLPrimitive *primitive, double dt)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to simulate OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;
	bool modified = false;
	particles->time += dt; // update time

	for(unsigned int i = 0; i < particles->num_particles; i++) {
		if((particles->time - particles->vertices[4*i+0].birth) > particles->properties.lifetime) { // check if particle lifetime expired
			initParticle(particles, i); // reinitialize it
			modified = true;
			continue;
		}
	}

	if(modified) { // synchronize particle effect with GPU if values were updated
		synchronizeOpenGLPrimitiveParticles(primitive);
	}

	return true;
}

/**
 * Synchronizes a particle effect primitive with its associated OpenGL buffer objects
 *
 * @param primitive			the particle effect primitive to be synchronized
 * @result					true if successful
 */
API bool synchronizeOpenGLPrimitiveParticles(OpenGLPrimitive *primitive)
{
	if(g_strcmp0(primitive->type, "particles") != 0) {
		LOG_ERROR("Failed to synchronized OpenGL particles: Primitive is not a particle effect");
		return false;
	}

	OpenGLParticles *particles = primitive->data;

	glBindBuffer(GL_ARRAY_BUFFER, particles->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleVertex) * particles->num_particles * 4, particles->vertices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particles->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ParticleSprite) * particles->num_particles, particles->sprites, GL_DYNAMIC_DRAW);

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
	glVertexAttribPointer(OPENGL_ATTRIBUTE_POSITION, 3, GL_FLOAT, false, sizeof(ParticleVertex), NULL + offsetof(ParticleVertex, position));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_POSITION);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_UV, 2, GL_FLOAT, false, sizeof(ParticleVertex), NULL + offsetof(ParticleVertex, corner));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_UV);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_NORMAL, 3, GL_FLOAT, false, sizeof(ParticleVertex), NULL + offsetof(ParticleVertex, velocity));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_NORMAL);
	glVertexAttribPointer(OPENGL_ATTRIBUTE_BIRTH, 1, GL_FLOAT, false, sizeof(ParticleVertex), NULL + offsetof(ParticleVertex, birth));
	glEnableVertexAttribArray(OPENGL_ATTRIBUTE_BIRTH);

	if(checkOpenGLError()) {
		return false;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particles->indexBuffer);
	glDrawElements(GL_TRIANGLES, particles->num_particles * 6, GL_UNSIGNED_INT, NULL);

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
	glDeleteBuffers(1, &particles->indexBuffer);
	free(particles->vertices);
	free(particles->sprites);
	free(particles->properties.positionMean);
	free(particles->properties.positionStd);
	free(particles->properties.velocityMean);
	free(particles->properties.velocityStd);
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
	float positionx = $(float, random, randomGaussian)($(float, linalg, getVector)(particles->properties.positionMean, 0), $(float, linalg, getVector)(particles->properties.positionStd, 0));
	float positiony = $(float, random, randomGaussian)($(float, linalg, getVector)(particles->properties.positionMean, 1), $(float, linalg, getVector)(particles->properties.positionStd, 1));
	float positionz = $(float, random, randomGaussian)($(float, linalg, getVector)(particles->properties.positionMean, 2), $(float, linalg, getVector)(particles->properties.positionStd, 2));
	float velocityx = $(float, random, randomGaussian)($(float, linalg, getVector)(particles->properties.velocityMean, 0), $(float, linalg, getVector)(particles->properties.velocityStd, 0));
	float velocityy = $(float, random, randomGaussian)($(float, linalg, getVector)(particles->properties.velocityMean, 1), $(float, linalg, getVector)(particles->properties.velocityStd, 1));
	float velocityz = $(float, random, randomGaussian)($(float, linalg, getVector)(particles->properties.velocityMean, 2), $(float, linalg, getVector)(particles->properties.velocityStd, 2));

	particles->vertices[4*i+0].corner[0] = 0.0f;
	particles->vertices[4*i+0].corner[1] = 0.0f;
	particles->vertices[4*i+1].corner[0] = 0.0f;
	particles->vertices[4*i+1].corner[1] = 1.0f;
	particles->vertices[4*i+2].corner[0] = 1.0f;
	particles->vertices[4*i+2].corner[1] = 0.0f;
	particles->vertices[4*i+3].corner[0] = 1.0f;
	particles->vertices[4*i+3].corner[1] = 1.0f;

	for(int j = 0; j < 4; j++) {
		particles->vertices[4*i+j].position[0] = positionx;
		particles->vertices[4*i+j].position[1] = positiony;
		particles->vertices[4*i+j].position[2] = positionz;
		particles->vertices[4*i+j].velocity[0] = velocityx;
		particles->vertices[4*i+j].velocity[1] = velocityy;
		particles->vertices[4*i+j].velocity[2] = velocityz;
		particles->vertices[4*i+j].birth = particles->time;
	}
}
