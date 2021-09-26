/*
 * Provides opengl functionality. Right now it doesn't do much because
 * glew is being used.
 *
 */


#include <stdlib.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "..\\include\\stb_image.h"
#include "win32_main.h"

static inline void GLClearErrors() {
	while(glGetError() != GL_NO_ERROR);
}

static inline bool GLCheckError(const char* file_name, 
								const char* func_name,
								int line_num) {
	unsigned int num_errors = 0;
	GLenum error_code = glGetError();
	bool is_error = error_code != GL_NO_ERROR;
	if(is_error) {
		// PRINT HEADER
		fprintf(stderr, "[OPENGL ERROR]:\n");
		fprintf(stderr, "file_name: ");
		fprintf(stderr, file_name);
		fprintf(stderr, "\nfunc_name: ");
		fprintf(stderr, func_name);
		fprintf(stderr, "\nline_num : ");
		fprintf(stderr, "%d\n", line_num);
		fprintf(stderr, "err_codes: [");

		while(error_code != GL_NO_ERROR) {
			num_errors++;
			fprintf(stderr, "%d, ", error_code);
		}
		fprintf(stderr, "]\n\n");
	}

	return (num_errors == 0);
}

#if DEBUG
#define GLCall(x) GLClearErrors();\
		  x;\
		  assert(GLCheckError(__FILE__, #x, __LINE__));
#else
#define GLCall(x) x;
#endif

INTERNAL unsigned int LoadTexture(char* texture_file_path) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	unsigned char *image_data = stbi_load(texture_file_path, &width, &height, &nrChannels, 0); 

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	return tex;
}

INTERNAL unsigned int LoadShaderProgram(char* vert_file, char* frag_file) {
	Win32FileResult vert = Win32ReadEntireFile(vert_file);
	Win32FileResult frag = Win32ReadEntireFile(frag_file);

	unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertex_shader, 1, &(const char*)vert.file, NULL);
	glCompileShader(vertex_shader);
	int success;
	char log[512];
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(vertex_shader, 512, NULL, log);
		fprintf(stderr, "[OPENGL SHADER ERROR]:\n");
		fprintf(stderr, "file_name: ");
		fprintf(stderr, __FILE__);
		fprintf(stderr, "\nshader file:");
		fprintf(stderr, vert_file);
		fprintf(stderr, "\nfunc_name: ");
		fprintf(stderr, __func__);
		fprintf(stderr, "\nline_num : ");
		fprintf(stderr, "%d\n", __LINE__);
		fprintf(stderr, log);
		fprintf(stderr, "\n\n");
	}
	glShaderSource(fragment_shader, 1, &(const char*)frag.file, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(fragment_shader, 512, NULL, log);
		fprintf(stderr, "[OPENGL SHADER ERROR]:\n");
		fprintf(stderr, "file_name: ");
		fprintf(stderr, __FILE__);
		fprintf(stderr, "shader file:");
		fprintf(stderr, vert_file);
		fprintf(stderr, "\nfunc_name: ");
		fprintf(stderr, __func__);
		fprintf(stderr, "\nline_num : ");
		fprintf(stderr, "%d\n", __LINE__);
		fprintf(stderr, log);
		fprintf(stderr, "\n\n");
	}

	GLCall(unsigned int shader_program = glCreateProgram());
	GLCall(glAttachShader(shader_program, vertex_shader));
	GLCall(glAttachShader(shader_program, fragment_shader));
	GLCall(glLinkProgram(shader_program));

	Win32FreeFileResult(&vert);
	Win32FreeFileResult(&frag);

	return shader_program;
}
