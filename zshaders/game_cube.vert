#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color_in;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 color_to_frag;

void main() {
	gl_Position = projection * view * model * vec4(pos.x, pos.y, pos.z, 1.0f);
	color_to_frag = color_in;
}
