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
#extension GL_EXT_texture_array : enable

uniform mat4 modelNormal;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform vec4 lightColor;
uniform float ambient;
uniform float specular;
uniform int lodLevel;
uniform int enableFragmentMorph;

uniform sampler2D normals;
uniform sampler2D parentNormals;
uniform sampler2D texture;
uniform sampler2D parentTexture;
uniform vec2 parentOffset;
uniform int heightmapWidth; // NOTE: textures are 2 pixels larger than this
uniform int heightmapHeight; // NOTE: textures are 2 pixels larger than this

varying vec3 world_position;
varying vec4 world_color;
varying vec2 world_grid;
varying float world_height;
varying float world_morph;

// some globals
int textureWidth = heightmapWidth + 2;
int textureHeight = heightmapHeight + 2;
vec2 textureDimension = vec2(textureWidth - 1, textureHeight - 1);
vec2 textureDimensionInv = vec2(1.0, 1.0) / textureDimension;

vec3 getNormal(in float morph)
{
	vec2 gridShift = world_grid + vec2(1.0, 1.0); // account for the fact that textures are 2 pixels larger
	vec2 uvShift = gridShift * textureDimensionInv;
	
	vec2 parentGridShift = 0.5 * world_grid + parentOffset * vec2(heightmapWidth - 1, heightmapHeight - 1) + vec2(1.0, 1.0);
	vec2 parentUvShift = parentGridShift * textureDimensionInv;
	
	vec3 normal = texture2D(normals, uvShift).xyz;
	vec3 parentNormal = texture2D(parentNormals, parentUvShift).xyz;
	
	return normalize(mix(normal, parentNormal, morph));
}

vec4 getColor(in vec2 uv, in float morph)
{
	vec4 value = texture2D(texture, uv);

	vec2 parentUV = 0.5 * uv + parentOffset;
	vec4 parentValue = texture2D(parentTexture, parentUV); 

	return mix(value, parentValue, morph);
}

vec4 phongAmbient(in vec4 textureColor)
{
	vec4 ambientColor = textureColor;
	ambientColor.xyz *= ambient;
	return ambientColor;
}

vec4 phongDiffuse(in vec4 textureColor, in vec3 pos2light, in vec3 normal)
{
	vec4 diffuseColor = textureColor * lightColor;
	diffuseColor.xyz *= clamp(dot(pos2light, normal), 0.0, 1.0);
	return diffuseColor;
}

vec4 phongSpecular(in vec3 pos2light, in vec3 pos2cam, in vec3 normal)
{
	vec3 reflectedLight = -reflect(pos2light, normal);
	
	float shininess = 100.0;
	float specProd = clamp(dot(reflectedLight, pos2cam), 0.0, 1.0);
	
	vec4 specularColor = lightColor;
	specularColor.xyz *= specular * pow(specProd, shininess);
	return specularColor;
}

void main()
{
	vec2 uv = world_grid / vec2(heightmapWidth - 1, heightmapHeight - 1);
	float fragmentMorph = world_morph * enableFragmentMorph;
	vec3 normal = getNormal(fragmentMorph);
	vec4 textureColor = getColor(uv, fragmentMorph);
	
	vec3 pos2light = normalize(lightPosition - world_position);
	vec3 pos2cam = normalize(cameraPosition - world_position);
	
	vec4 ac = phongAmbient(textureColor);
	vec4 dc = phongDiffuse(textureColor, pos2light, normal);
	vec4 sc = phongSpecular(pos2light, pos2cam, normal);

	// gl_FragColor = vec4(normal, 1.0);
	gl_FragColor = clamp(ac + dc + sc, 0.0, 1.0);
	// gl_FragColor = vec4(world_height, 0.25, 0.25, 1.0);
}
