#version 120

uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform vec4 lightColor;
uniform float ambient;
uniform float specular;

varying vec3 world_position;
varying vec3 world_normal;
varying vec4 world_color;

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
