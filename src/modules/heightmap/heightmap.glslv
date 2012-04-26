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
uniform mat4 camera;
uniform mat4 perspective;
uniform sampler2D heights;
uniform int heightmapWidth;
uniform int heightmapHeight;
uniform sampler2D normals;

attribute vec2 uv;

varying vec3 world_position;
varying vec3 world_normal;
varying vec4 world_color;
varying vec2 world_uv;
varying float world_height;

void main()
{
	vec2 heightmapPosition = uv / vec2(heightmapWidth, heightmapHeight);
	float height = texture2D(heights, heightmapPosition).x;
	vec4 pos4 = vec4(heightmapPosition.x, height, heightmapPosition.y, 1.0);
	vec3 norm3 = texture2D(normals, heightmapPosition).xyz;
	vec4 norm4 = vec4(heightmapWidth * norm3.x, norm3.y, heightmapHeight * norm3.z, 1.0); // compensate scaling of heightmap to [0,1]x[0,1]
	vec4 worldpos4 = model * pos4;
	vec4 worldnorm4 = modelNormal * norm4;

	world_position = worldpos4.xyz / worldpos4.w;
	world_normal = normalize(worldnorm4.xyz / worldnorm4.w);
	world_color = vec4(heightmapPosition.x, height, heightmapPosition.y, 1.0);
	world_uv = heightmapPosition;
	world_height = height;
		
	gl_Position = perspective * camera * worldpos4;
}
