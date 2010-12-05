#version 120

uniform mat4 camera;
uniform mat4 perspective;

attribute vec3 position;
attribute vec3 normal;
attribute vec4 color;

varying vec3 world_position;
varying vec3 world_normal;
varying vec4 world_color;

void main()
{
	world_position = position;
	world_normal = normal;
	world_color = color;
	
	vec4 pos4 = vec4(position.x, position.y, position.z, 1.0);
	gl_Position = perspective * camera * pos4;
}
