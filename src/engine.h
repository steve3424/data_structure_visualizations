#if !defined(ENGINE_H)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#define GLEW_STATIC
#include "..\include\glew.h"
#include "..\\include\\glm\\glm.hpp"
#include "..\\include\\glm\\gtc\\matrix_transform.hpp"
#include "..\\include\\glm\\gtc\\type_ptr.hpp"

#define INTERNAL static
#define LOCALPERSIST static
#define GLOBAL static
#define PI 3.14159265359f
#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)
#define ArrayCount(array) (sizeof(array)/sizeof(array[0]))
#define STRUCT_MEMBER_SIZE(type, member) sizeof(((type*)0)->member)
#define MAX_DIGITS 100
#define VERTICES_PER_CUBE (sizeof(GameCube) / sizeof(Vertex))

// TODO: Maybe parameterize node width here in a #define

typedef enum {
	INSERTION_SORT,
	AVL_TREE,
	NUM_VIEWS // THIS NEEDS TO BE THE LAST ENUM IN THE LIST
} View;

typedef struct {
	bool is_down;
	int repeat_count;
} GameButtonState;

// buttons are for DVORAK layout right now
typedef struct GameInput {
	int num_buttons = 22;
	union {
		GameButtonState buttons[22];
		struct {
			GameButtonState comma;
			GameButtonState a;
			GameButtonState o;
			GameButtonState e;
			GameButtonState s;
			GameButtonState p;
			GameButtonState w;
			GameButtonState v;
			GameButtonState num_0;
			GameButtonState num_1;
			GameButtonState num_2;
			GameButtonState num_3;
			GameButtonState num_4;
			GameButtonState num_5;
			GameButtonState num_6;
			GameButtonState num_7;
			GameButtonState num_8;
			GameButtonState num_9;

			GameButtonState arrow_up;
			GameButtonState arrow_down;
			GameButtonState arrow_left;
			GameButtonState arrow_right;
		};
	};
} GameInput;

typedef struct {
	uint64_t storage_size;
	void* storage;
} GameMemory;

typedef struct {
	float x; 
	float y;
	float z;
} GameCamera;

typedef struct Vertex {
	union {
		float pos[3];
		struct {
			float x;
			float y;
			float z;
		};
	};

	union {
		float rgb[3];
		struct {
			float r;
			float g;
			float b;
		};
	};
} Vertex;

typedef struct {
	unsigned int vao;
	unsigned int vbo;
	unsigned int ibo;
	unsigned int texture;
	unsigned int shader;
} GameBackground;

typedef struct {
	Vertex cube_vertices[24];
	Vertex digit_vertices[20];
	Vertex line_vertices[2];
} GameCube;

typedef struct {
	View current_view;
	void** data_structures;

	int window_width;
	int window_height;
} GameState;

INTERNAL void           GenDigit(const float x, const float y, const float z, const int val, GameCube* cube);
INTERNAL GameCube       GenCube(const float x, const float y, const float z, const int val, float r, float g, float b);
INTERNAL GameBackground GenBackgroundBuffer();
#define ENGINE_H
#endif
