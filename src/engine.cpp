/*
 * This is the actual game engine. It manages the current view and
 * it interacts with the different data structure files.
 *
 * It also provides some generic geometry types that are used in different
 * data structures e.g. GameCube, GameCamera, ..., in "engine.h"
 * 
 * The current pattern is that each data structure will be in a separate
 * .cpp file. It will own a vertex buffer, GameCamera, shader, background, etc.
 * It should have an interface that is callable from the switch block in 
 * GameUpdateAndRender below that will take care of all updating and
 * drawing of the visualization
 *
 * The camera update could also be moved out to the individual data
 * structure files if something fancy is to be done on a particular visualization
 *
 */

// TODO: I'm not sure if I should make a single draw call at the end of 
//       the switch or have it in the separate ds files

#include "engine.h"
#include "stdlib.h"

INTERNAL void UpdateView(View* current_view, const GameInput* input) {
	int temp_current_view = (int)(*current_view);
	int num_views = (int)NUM_VIEWS;
	if(input->w.is_down) {
		if(0 < temp_current_view) {
			temp_current_view--;
		}
		else {
			temp_current_view = num_views - 1;
		}
	}
	else if(input->v.is_down) {
		++temp_current_view %= num_views;
	}
	
	*current_view = (View)temp_current_view;
}

INTERNAL inline void UpdateCamera(GameCamera* camera, const GameInput* input) {
	if(input->arrow_right.is_down) {
		camera->x -= 0.10f;
	}
	if(input->arrow_left.is_down) {
		camera->x += 0.10f;
	}
	if(input->arrow_up.is_down) {
		camera->y -= 0.10f;
	}
	if(input->arrow_down.is_down) {
		camera->y += 0.10f;
	}
	if(input->comma.is_down) {
		camera->z += 0.10f;
	}
	if(input->o.is_down) {
		camera->z -= 0.10f;
	}
}

INTERNAL void GameUpdateAndRender(GameMemory* memory, GameInput* input) {
	GameState* game_state = (GameState*)memory->storage;

	UpdateView(&game_state->current_view, input);
	
	switch(game_state->current_view) {
		case INSERTION_SORT: 
		{
			ISort* isort = (ISort*)game_state->data_structures[INSERTION_SORT];
			if(!isort) {
				game_state->data_structures[INSERTION_SORT] = ISort_Init();
				isort = (ISort*)game_state->data_structures[INSERTION_SORT];
			}
			UpdateCamera(&isort->camera, input);
			ISort_Update(isort, input);
			ISort_Draw(isort, (float)game_state->window_width, (float)game_state->window_height);
		} break;

		case AVL_TREE:
		{
			AVLTree* avl_tree = (AVLTree*)game_state->data_structures[AVL_TREE];
			if(!avl_tree) {
				game_state->data_structures[AVL_TREE] = AVLTree_Init();
				avl_tree = (AVLTree*)game_state->data_structures[AVL_TREE];
			}
			UpdateCamera(&avl_tree->camera, input);
			AVLTree_Update(avl_tree, input);
			AVLTree_Draw(avl_tree, (float)game_state->window_width, (float)game_state->window_height);
		} break;

		default:
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.694f, 0.612f, 0.851f, 1.0f);
		} break;
	}
}

/* Returns a gamebackground object which contains
 * all of the VAO buffers that are to be bound
 * in opengl before drawing the background
 *
 */
INTERNAL GameBackground GenBackgroundBuffer() {
	// 2 triangles for background quad
	float vertices[] = {
		-1.0f,  1.0f, -0.6f, 0.0f, 1.0f,
		 1.0f,  1.0f, -0.6f, 1.0f, 1.0f,
		 1.0f, -1.0f, -0.6f, 1.0f, 0.0f,
		-1.0f, -1.0f, -0.6f, 0.0f, 0.0f
	};
	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int vao;
	GLCall(glCreateVertexArrays(1, &vao));
	GLCall(glBindVertexArray(vao));

	unsigned int vbo;
	GLCall(glGenBuffers(1, &vbo));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GLCall(glBufferData(GL_ARRAY_BUFFER, 20*sizeof(float), vertices, GL_STATIC_DRAW));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float))));
	GLCall(glEnableVertexAttribArray(1));

	unsigned int ibo;
	GLCall(glGenBuffers(1, &ibo));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(unsigned int), indices, GL_STATIC_DRAW));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));

	GameBackground gb;
	gb.vao = vao;
	gb.vbo = vbo;
	gb.ibo = ibo;

	return gb;
}

void GameCube_SetColor(GameCube* cube, float r, float g, float b) {
	assert(cube);

	int num_cube_vertices = STRUCT_MEMBER_SIZE(GameCube, cube_vertices) / sizeof(Vertex);
	for(int j = 0; j < num_cube_vertices; ++j) {
		cube->cube_vertices[j].r = r;
		cube->cube_vertices[j].g = g;
		cube->cube_vertices[j].b = b;
	}
}

INTERNAL void GenDigit(const float x, const float y, const float z, const int val, GameCube* cube) {
	assert(cube);
	assert(0 <= val);
	assert(val <= 99);

	Vertex digit_vertices[6];
	digit_vertices[0].pos[0] = -0.15f + x;
	digit_vertices[0].pos[1] =  0.3f  + y;
	digit_vertices[0].pos[2] =  0.0f  + z;
	digit_vertices[0].rgb[0] = 0.0f;
	digit_vertices[0].rgb[1] = 1.0f;
	digit_vertices[0].rgb[2] = 0.0f;
	digit_vertices[1].pos[0] = 0.15f  + x;
	digit_vertices[1].pos[1] = 0.4f   + y;
	digit_vertices[1].pos[2] = 0.0f   + z;
	digit_vertices[1].rgb[0] = 0.0f;
	digit_vertices[1].rgb[1] = 1.0f;
	digit_vertices[1].rgb[2] = 0.0f;
	digit_vertices[2].pos[0] = 0.15f  + x;
	digit_vertices[2].pos[1] = 0.0f   + y;
	digit_vertices[2].pos[2] = 0.0f   + z;
	digit_vertices[2].rgb[0] = 0.0f;
	digit_vertices[2].rgb[1] = 1.0f;
	digit_vertices[2].rgb[2] = 0.0f;
	digit_vertices[3].pos[0] =  0.15f + x;
	digit_vertices[3].pos[1] = -0.3f  + y;
	digit_vertices[3].pos[2] =  0.0f  + z;
	digit_vertices[3].rgb[0] = 0.0f;
	digit_vertices[3].rgb[1] = 1.0f;
	digit_vertices[3].rgb[2] = 0.0f;
	digit_vertices[4].pos[0] = -0.15f + x;
	digit_vertices[4].pos[1] = -0.4f  + y;
	digit_vertices[4].pos[2] =  0.0f  + z;
	digit_vertices[4].rgb[0] = 0.0f;
	digit_vertices[4].rgb[1] = 1.0f;
	digit_vertices[4].rgb[2] = 0.0f;
	digit_vertices[5].pos[0] = -0.15f + x;
	digit_vertices[5].pos[1] =  0.0f  + y;
	digit_vertices[5].pos[2] =  0.0f  + z;
	digit_vertices[5].rgb[0] = 0.0f;
	digit_vertices[5].rgb[1] = 1.0f;
	digit_vertices[5].rgb[2] = 0.0f;
 
	float x_shift = 0.0f;
	int num_digits = 1;
	int digits[2] = {val, 0};
	if(10 <= val) {
		x_shift = -0.23f;
		num_digits = 2;
		digits[0] = val / 10;
		digits[1] = val % 10;
	}

	int write_index = 0;
	for(int i = 0; i < num_digits; ++i) {
		switch(digits[i]) {
			case 0: 
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 1: 
			{
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 2: 
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 3: 
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 4: 
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 5: 
			{
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 6: 
			{
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 7: 
			{
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 8: 
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			case 9: 
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[5];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[2];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;

			// digit is 0
			default:
			{
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[1];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[3];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[4];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
				cube->digit_vertices[write_index] = digit_vertices[0];
				cube->digit_vertices[write_index++].pos[0] += x_shift;
			} break;
		}

		x_shift *= -1.0f;
	}
}

INTERNAL GameCube GenCube(const float x, const float y, const float z, const int val, float r, float g, float b) {
	GameCube cube = {0};
	cube.cube_vertices[0].pos[0] = x - 0.5f;
	cube.cube_vertices[0].pos[1] = y + 0.5f;
	cube.cube_vertices[0].pos[2] = z + 0.5f;
	cube.cube_vertices[0].rgb[0] = r;
	cube.cube_vertices[0].rgb[1] = g;
	cube.cube_vertices[0].rgb[2] = b;
	cube.cube_vertices[1].pos[0] = x + 0.5f;
	cube.cube_vertices[1].pos[1] = y + 0.5f;
	cube.cube_vertices[1].pos[2] = z + 0.5f;
	cube.cube_vertices[1].rgb[0] = r;
	cube.cube_vertices[1].rgb[1] = g;
	cube.cube_vertices[1].rgb[2] = b;

	cube.cube_vertices[2].pos[0] = x + 0.5f;
	cube.cube_vertices[2].pos[1] = y + 0.5f;
	cube.cube_vertices[2].pos[2] = z + 0.5f;
	cube.cube_vertices[2].rgb[0] = r;
	cube.cube_vertices[2].rgb[1] = g;
	cube.cube_vertices[2].rgb[2] = b;
	cube.cube_vertices[3].pos[0] = x + 0.5f;
	cube.cube_vertices[3].pos[1] = y - 0.5f;
	cube.cube_vertices[3].pos[2] = z + 0.5f;
	cube.cube_vertices[3].rgb[0] = r;
	cube.cube_vertices[3].rgb[1] = g;
	cube.cube_vertices[3].rgb[2] = b;

	cube.cube_vertices[4].pos[0] = x + 0.5f;
	cube.cube_vertices[4].pos[1] = y - 0.5f;
	cube.cube_vertices[4].pos[2] = z + 0.5f;
	cube.cube_vertices[4].rgb[0] = r;
	cube.cube_vertices[4].rgb[1] = g;
	cube.cube_vertices[4].rgb[2] = b;
	cube.cube_vertices[5].pos[0] = x - 0.5f;
	cube.cube_vertices[5].pos[1] = y - 0.5f;
	cube.cube_vertices[5].pos[2] = z + 0.5f;
	cube.cube_vertices[5].rgb[0] = r;
	cube.cube_vertices[5].rgb[1] = g;
	cube.cube_vertices[5].rgb[2] = b;

	cube.cube_vertices[6].pos[0] = x - 0.5f;
	cube.cube_vertices[6].pos[1] = y - 0.5f;
	cube.cube_vertices[6].pos[2] = z + 0.5f;
	cube.cube_vertices[6].rgb[0] = r;
	cube.cube_vertices[6].rgb[1] = g;
	cube.cube_vertices[6].rgb[2] = b;
	cube.cube_vertices[7].pos[0] = x - 0.5f;
	cube.cube_vertices[7].pos[1] = y + 0.5f;
	cube.cube_vertices[7].pos[2] = z + 0.5f;
	cube.cube_vertices[7].rgb[0] = r;
	cube.cube_vertices[7].rgb[1] = g;
	cube.cube_vertices[7].rgb[2] = b;

	cube.cube_vertices[8].pos[0] = x - 0.5f;
	cube.cube_vertices[8].pos[1] = y + 0.5f;
	cube.cube_vertices[8].pos[2] = z - 0.5f;
	cube.cube_vertices[8].rgb[0] = r;
	cube.cube_vertices[8].rgb[1] = g;
	cube.cube_vertices[8].rgb[2] = b;
	cube.cube_vertices[9].pos[0] = x + 0.5f;
	cube.cube_vertices[9].pos[1] = y + 0.5f;
	cube.cube_vertices[9].pos[2] = z - 0.5f;
	cube.cube_vertices[9].rgb[0] = r;
	cube.cube_vertices[9].rgb[1] = g;
	cube.cube_vertices[9].rgb[2] = b;

	cube.cube_vertices[10].pos[0] = x + 0.5f;
	cube.cube_vertices[10].pos[1] = y + 0.5f;
	cube.cube_vertices[10].pos[2] = z - 0.5f;
	cube.cube_vertices[10].rgb[0] = r;
	cube.cube_vertices[10].rgb[1] = g;
	cube.cube_vertices[10].rgb[2] = b;
	cube.cube_vertices[11].pos[0] = x + 0.5f;
	cube.cube_vertices[11].pos[1] = y - 0.5f;
	cube.cube_vertices[11].pos[2] = z - 0.5f;
	cube.cube_vertices[11].rgb[0] = r;
	cube.cube_vertices[11].rgb[1] = g;
	cube.cube_vertices[11].rgb[2] = b;

	cube.cube_vertices[12].pos[0] = x + 0.5f;
	cube.cube_vertices[12].pos[1] = y - 0.5f;
	cube.cube_vertices[12].pos[2] = z - 0.5f;
	cube.cube_vertices[12].rgb[0] = r;
	cube.cube_vertices[12].rgb[1] = g;
	cube.cube_vertices[12].rgb[2] = b;
	cube.cube_vertices[13].pos[0] = x - 0.5f;
	cube.cube_vertices[13].pos[1] = y - 0.5f;
	cube.cube_vertices[13].pos[2] = z - 0.5f;
	cube.cube_vertices[13].rgb[0] = r;
	cube.cube_vertices[13].rgb[1] = g;
	cube.cube_vertices[13].rgb[2] = b;

	cube.cube_vertices[14].pos[0] = x - 0.5f;
	cube.cube_vertices[14].pos[1] = y - 0.5f;
	cube.cube_vertices[14].pos[2] = z - 0.5f;
	cube.cube_vertices[14].rgb[0] = r;
	cube.cube_vertices[14].rgb[1] = g;
	cube.cube_vertices[14].rgb[2] = b;
	cube.cube_vertices[15].pos[0] = x - 0.5f;
	cube.cube_vertices[15].pos[1] = y + 0.5f;
	cube.cube_vertices[15].pos[2] = z - 0.5f;
	cube.cube_vertices[15].rgb[0] = r;
	cube.cube_vertices[15].rgb[1] = g;
	cube.cube_vertices[15].rgb[2] = b;

	cube.cube_vertices[16].pos[0] = x - 0.5f;
	cube.cube_vertices[16].pos[1] = y + 0.5f;
	cube.cube_vertices[16].pos[2] = z + 0.5f;
	cube.cube_vertices[16].rgb[0] = r;
	cube.cube_vertices[16].rgb[1] = g;
	cube.cube_vertices[16].rgb[2] = b;
	cube.cube_vertices[17].pos[0] = x - 0.5f;
	cube.cube_vertices[17].pos[1] = y + 0.5f;
	cube.cube_vertices[17].pos[2] = z - 0.5f;
	cube.cube_vertices[17].rgb[0] = r;
	cube.cube_vertices[17].rgb[1] = g;
	cube.cube_vertices[17].rgb[2] = b;

	cube.cube_vertices[18].pos[0] = x + 0.5f;
	cube.cube_vertices[18].pos[1] = y + 0.5f;
	cube.cube_vertices[18].pos[2] = z + 0.5f;
	cube.cube_vertices[18].rgb[0] = r;
	cube.cube_vertices[18].rgb[1] = g;
	cube.cube_vertices[18].rgb[2] = b;
	cube.cube_vertices[19].pos[0] = x + 0.5f;
	cube.cube_vertices[19].pos[1] = y + 0.5f;
	cube.cube_vertices[19].pos[2] = z - 0.5f;
	cube.cube_vertices[19].rgb[0] = r;
	cube.cube_vertices[19].rgb[1] = g;
	cube.cube_vertices[19].rgb[2] = b;

	cube.cube_vertices[20].pos[0] = x + 0.5f;
	cube.cube_vertices[20].pos[1] = y - 0.5f;
	cube.cube_vertices[20].pos[2] = z + 0.5f;
	cube.cube_vertices[20].rgb[0] = r;
	cube.cube_vertices[20].rgb[1] = g;
	cube.cube_vertices[20].rgb[2] = b;
	cube.cube_vertices[21].pos[0] = x + 0.5f;
	cube.cube_vertices[21].pos[1] = y - 0.5f;
	cube.cube_vertices[21].pos[2] = z - 0.5f;
	cube.cube_vertices[21].rgb[0] = r;
	cube.cube_vertices[21].rgb[1] = g;
	cube.cube_vertices[21].rgb[2] = b;

	cube.cube_vertices[22].pos[0] = x - 0.5f;
	cube.cube_vertices[22].pos[1] = y - 0.5f;
	cube.cube_vertices[22].pos[2] = z + 0.5f;
	cube.cube_vertices[22].rgb[0] = r;
	cube.cube_vertices[22].rgb[1] = g;
	cube.cube_vertices[22].rgb[2] = b;
	cube.cube_vertices[23].pos[0] = x - 0.5f;
	cube.cube_vertices[23].pos[1] = y - 0.5f;
	cube.cube_vertices[23].pos[2] = z - 0.5f;
	cube.cube_vertices[23].rgb[0] = r;
	cube.cube_vertices[23].rgb[1] = g;
	cube.cube_vertices[23].rgb[2] = b;

	GenDigit(x, y, z, val, &cube);

	return cube;
}
