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

#version 120

uniform mat4 model;
uniform mat4 modelNormal;
uniform mat4 perspective;
uniform mat4 camera;
uniform float time;
uniform float lifetime;
uniform float startSize;
uniform float endSize;
uniform float aspectRatio;

attribute vec3 position;
attribute vec2 uv;
attribute vec3 normal;
attribute float birth;

varying vec3 world_position;
varying vec2 world_uv;

vec3 computePosition(float dt)
{
	vec3 velocity = normal;
	return position + velocity * dt;
}

float computeSize(float lifep)
{
	return mix(startSize, endSize, lifep);
}

void main()
{
	mat4 perspectiveCamera = perspective * camera;

	float dt = time - birth;
	float lifep = dt / lifetime;

	vec3 currentPosition = computePosition(dt);
	float size = computeSize(lifep);
	
	vec4 pos4 = vec4(currentPosition.x, currentPosition.y, currentPosition.z, 1.0);
	vec4 worldpos4 = model * pos4;
	world_position = worldpos4.xyz / worldpos4.w;
	world_uv = uv;
	vec4 screenpos4 = perspectiveCamera * worldpos4;

	vec2 corner = size * (uv - vec2(0.5, 0.5));
	corner.y *= aspectRatio;
	screenpos4.xy += corner;
	
	gl_Position = screenpos4;
}
