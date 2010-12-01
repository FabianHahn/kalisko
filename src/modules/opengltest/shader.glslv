#version 120

uniform mat4 camera;
uniform mat4 perspective;

attribute vec3 position;
attribute vec3 normal;
attribute vec4 color;

varying vec4 v_color;

void main()
{
	v_color = color;
	
	vec4 pos4 = vec4(position.x, position.y, position.z, 1.0);
	gl_Position = perspective * camera * pos4;
}
