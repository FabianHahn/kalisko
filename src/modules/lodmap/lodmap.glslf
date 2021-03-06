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
uniform int heightmapWidth;
uniform int heightmapHeight;

varying vec3 world_position;
varying vec4 world_color;
varying vec2 world_grid;
varying float world_height;
varying float world_morph;

// some globals
vec2 textureDimension = vec2(heightmapWidth - 1, heightmapHeight - 1);
vec2 textureDimensionInv = vec2(1.0, 1.0) / textureDimension;
float inverseScaleTransform = pow(2.0, float(lodLevel)); // this is the transform that we'll use to revert the x and z model transform for the normals

vec3 computeWorldNormal(in sampler2D texture, in vec2 uv)
{
 	// look up our normals texture and unpack it
	vec3 normalNode = 2 * texture2D(texture, uv).xyz - vec3(1.0, 1.0, 1.0);
	
	// revert the scaling done by the world transformation, then apply the world transformation
	vec4 normalWorld = modelNormal * vec4(inverseScaleTransform * normalNode.x, normalNode.y, inverseScaleTransform * normalNode.z, 1.0);
	
	 // transform back from homogeneous coordinates
	vec3 normal = normalize(normalWorld.xyz / normalWorld.w);
	
	return normal;
}

vec3 getNormal(in vec2 uv, in float morph)
{
	// The important issue to note here is that we're scaling the original heightmap node with range [0,1]x[0,1] up to whatever
	// quadtree level in the LOD map we're at in both x and z direction. The problem is that the OpenGL model is too intelligent
	// since it will try to apply the inverse transform to the normal vectors. Now recall that we're actually leaving out every
	// other normal information we have when scaling up a node one level and reading the corresponding node's normal texture,
	// which means that this transformation was already implicitly done. In order to prevent it from being applied twice, we need
	// to reverse it here by scaling up all normals in x and z direction (since the inverse transform scales them down). The y
	// transformation (that stems from the heightRatio parameter of the data source) will be correctly applied by the normal
	// transform, however, so we better leave that one alone. 

	// first do it for our own normal texture
	vec3 normal = computeWorldNormal(normals, uv);

	// now do the same for the parent texture to which we interpolate
	vec2 parentUV = 0.5 * uv + parentOffset; // compute the UVs of this vertex inside the parent node's normal texture
	vec3 parentNormal = computeWorldNormal(parentNormals, parentUV);
	
	return normalize(mix(normal, parentNormal, morph)); // now mix them according to the vertex morph
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
	vec2 uv = textureDimensionInv * world_grid;
	float fragmentMorph = world_morph * enableFragmentMorph;
	vec3 normal = getNormal(uv, fragmentMorph);
	vec4 textureColor = getColor(uv, fragmentMorph);
	
	vec3 pos2light = normalize(lightPosition - world_position);
	vec3 pos2cam = normalize(cameraPosition - world_position);
	
	vec4 ac = phongAmbient(textureColor);
	vec4 dc = phongDiffuse(textureColor, pos2light, normal);
	vec4 sc = phongSpecular(pos2light, pos2cam, normal);

	// gl_FragColor = vec4(0.5 * world_height, 0.25 * world_height, 0.25 * lodLevel, 1.0);
	gl_FragColor = clamp(ac + dc + sc, 0.0, 1.0);
	// gl_FragColor = vec4(0.5 * normal + 0.5, 1.0);
	// gl_FragColor = vec4(world_height, 0.25, 0.25, 1.0);
}
