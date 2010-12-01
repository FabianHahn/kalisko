#version 120

attribute vec3 position;
attribute vec3 normal;
attribute vec4 color;

varying vec4 v_color;

void main()
{
	gl_Position.xyz = position;
}
