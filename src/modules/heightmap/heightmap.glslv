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

attribute vec2 uv;

varying vec3 world_position;
varying vec3 world_normal;
varying vec4 world_color;
varying vec2 world_uv;

void main()
{
	vec2 heightmapPosition = uv / vec2(heightmapWidth - 1.0, heightmapHeight - 1.0);
	float height = texture2D(heights, heightmapPosition).x;
	vec4 pos4 = vec4(uv.x, height, uv.y, 1.0);
	vec4 norm4 = vec4(0.0, 1.0, 0.0, 1.0);
	vec4 worldpos4 = model * pos4;
	vec4 worldnorm4 = modelNormal * norm4;

	world_position = worldpos4.xyz / worldpos4.w;
	world_normal = worldnorm4.xyz / worldnorm4.w;
	world_color = vec4(heightmapPosition.x, height, heightmapPosition.y, 1.0);
	world_uv = uv;
		
	gl_Position = perspective * camera * worldpos4;
}
