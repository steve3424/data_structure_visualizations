#version 330 core

in vec3 color_to_frag;

out vec4 final_color;

void main() {
	final_color = vec4(color_to_frag, 1.0);
}
