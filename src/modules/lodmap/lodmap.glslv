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
uniform mat4 camera;
uniform mat4 perspective;

// heightmap uniforms
uniform sampler2D heights;
uniform int heightmapWidth; // NOTE: textures are 2 pixels larger than this
uniform int heightmapHeight; // NOTE: textures are 2 pixels larger than this

// lodmap uniforms
uniform int lodLevel;
uniform float baseRange;
uniform float morphStartFactor;
uniform vec3 viewerPosition;

attribute vec2 uv;

varying vec3 world_position;
varying vec4 world_color;
varying vec2 world_uv;
varying float world_height;
varying float world_morph;

const float morphEndScaleFactor = 0.99;

// some globals
int textureWidth = heightmapWidth + 2;
int textureHeight = heightmapHeight + 2;
vec2 textureDimension = vec2(textureWidth - 1, textureHeight - 1);
vec2 textureDimensionInv = vec2(1.0, 1.0) / textureDimension;

vec2 morphVertex(in vec2 vertexGrid, in float morphFactor)
{
	vec2 isOdd = 2.0 * fract(0.5 * vertexGrid); // determine whether the x and y component of the grid vertex is odd
	return vertexGrid - isOdd * morphFactor; // morph it back for each odd component in the respective direction
}

vec4 texture2DBilinearGrid(in sampler2D texture, in vec2 uvGrid)
{
	vec2 uvGridShift = uvGrid + vec2(1.0, 1.0); // account for the fact that textures are 2 pixels larger
	vec2 uvGridTopLeft = floor(uvGridShift);
	vec2 uvTopLeft = uvGridTopLeft * textureDimensionInv; 
	
	// sample texture
    vec4 topLeft = texture2D(texture, uvTopLeft);
    vec4 topRight = texture2D(texture, uvTopLeft + vec2(textureDimensionInv.x, 0.0));
    vec4 bottomLeft = texture2D(texture, uvTopLeft + vec2(0.0, textureDimensionInv.y));
    vec4 bottomRight = texture2D(texture, uvTopLeft + textureDimensionInv);
    
    // bilinear interpolation
    vec2 ratio = uvGridShift - uvGridTopLeft;
    vec4 topX = mix(topLeft, topRight, ratio.x);
    vec4 bottomX = mix(bottomLeft, bottomRight, ratio.x);
    return mix(topX, bottomX, ratio.y);
}

float getLodDistance(in vec3 vertexWorld)
{
	return distance(viewerPosition, vertexWorld);
}

float getLodRange(in int level)
{
	float scale = pow(2.0, level);
	int nonnegative = int(level >= 0); // we need to return zero if the level is negative
	return nonnegative * baseRange * scale;
}

float getMorphFactor(in vec2 vertexGrid)
{
	// determine start and end LOD ranges for morphing
	float morphEnd = morphEndScaleFactor * getLodRange(lodLevel);
	float morphPreviousEnd = morphEndScaleFactor * getLodRange(lodLevel - 1);
	float morphStart = morphPreviousEnd + morphStartFactor * (morphEnd - morphPreviousEnd);
	
	// transform the grid vertex to model coordinates
	vec2 vertexModel = vertexGrid / vec2(heightmapWidth - 1, heightmapHeight - 1);
	
	// transform the grid vertex to world coordinates
	float vertexHeight = texture2D(heights, (vertexGrid + vec2(1.0, 1.0)) * textureDimensionInv).x; // account for the fact that textures are 2 pixels larger
	vec4 vertexWorld = model * vec4(vertexModel.x, vertexHeight, vertexModel.y, 1.0);
	
	// compute the lod distance for the vertex
	float lodDistance = getLodDistance(vertexWorld.xyz);
	
	// compute the morph factor for the vertex
	return clamp((lodDistance - morphStart) / (morphEnd - morphStart), 0.0, 1.0);
}

void main()
{
	vec2 vertexGrid = uv;

	// compute the morph factor for the vertex
	float morphFactor = getMorphFactor(vertexGrid);
	
	// morph the vertex
	vec2 morphedGrid = morphVertex(vertexGrid, morphFactor);
	
	// transform the morphed grid vertex to model coordinates
	vec2 morphedModel = morphedGrid / vec2(heightmapWidth - 1, heightmapHeight - 1);
	
	// transform the grid vertex to world coordinates
	float morphedHeight = texture2DBilinearGrid(heights, morphedGrid).x;
	vec4 morphedWorld = model * vec4(morphedModel.x, morphedHeight, morphedModel.y, 1.0);
	
	// store output values
	world_position = morphedWorld.xyz / morphedWorld.w;
	world_color = vec4(morphedHeight / 2.0, morphedHeight, 0.5, 1.0);
	world_uv = morphedModel;
	world_height = morphedHeight;
	world_morph = morphFactor;
		
	gl_Position = perspective * camera * morphedWorld;
}
