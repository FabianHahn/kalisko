/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2010, Kalisko Project Leaders
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

uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform vec4 lightColor;
uniform float ambient;
uniform float specular;

varying vec3 world_position;
varying vec3 world_normal;
varying vec4 world_color;
varying vec2 world_uv;

vec4 phongAmbient()
{
	return ambient * world_color;
}

vec4 phongDiffuse(in vec3 pos2light, in vec3 normal)
{
	return (world_color * lightColor) * clamp(dot(pos2light, normal), 0.0, 1.0);
}

vec4 phongSpecular(in vec3 pos2light, in vec3 pos2cam, in vec3 normal)
{
	vec3 reflectedLight = -reflect(pos2light, normal);
	
	float shininess = 100.0;
	float specProd = clamp(dot(reflectedLight, pos2cam), 0.0, 1.0);
	
	return specular * lightColor * pow(specProd, shininess);
}

void main()
{
	vec3 normal = normalize(world_normal);
	vec3 pos2light = normalize(lightPosition - world_position);
	vec3 pos2cam = normalize(cameraPosition - world_position);
	
	if(dot(normal, pos2cam) < 0) {
		normal = -normal;
	}
	
	vec4 ac = phongAmbient();
	vec4 dc = phongDiffuse(pos2light, normal);
	vec4 sc = phongSpecular(pos2light, pos2cam, normal);

	gl_FragColor = clamp(ac + dc + sc, 0.0, 1.0);
}
