#include <stdbool.h>
#include "engine.h"

#define INSERTION_SORT_SIZE 16
#define THRESHOLD 0.001f

// units is a unit cube 1.0f
static float units_per_second = 1.0f;
static float const frames_per_second = 60.0f;
static float const y_lift_val = 1.3f;

typedef enum {
	ISORT_INITIALIZING,
	ISORT_STATIC,
	ISORT_LIFTING_SELECTED_VALUE,
	ISORT_COMPARING,
	ISORT_SHIFTING_RIGHT,
	ISORT_SHIFTING_LEFT,
	ISORT_SHIFTING_DOWN,
	ISORT_PAUSED
} ISortState;

typedef struct {
	GameCube cube;
	int val;
	float x_dest;
	float x_vel;
	float y_dest;
	float y_vel;
	int start_index;
} ISortNode;

typedef struct {
	ISortState current_state;
	ISortState previous_state;
	int selected_val_index;
	int compare_val_index;
	ISortNode nodes[INSERTION_SORT_SIZE];

	GameCamera camera;
	unsigned int vbo;
	unsigned int shader;
	
	GameBackground background;
} ISort;

inline bool ISort_AnimationFinished(float const location, float const destination) {
	float diff = destination - location;
	if(diff < 0.0f) {
		diff *= -1.0f;
	}

	return diff <= THRESHOLD;
}

inline float ISort_SetVelocity(float const location, float const destination) {
	float dist = destination - location;
	float units_per_frame = units_per_second / frames_per_second;
	float frames_to_reach_dest = dist / units_per_frame;
	if(frames_to_reach_dest < 0.0f) {
		frames_to_reach_dest *= -1.0f;
	}
	int new_frames_to_reach_dest = (int)(frames_to_reach_dest + 1.0f);
	return dist / (float)new_frames_to_reach_dest;
}

ISort* ISort_Init() {
	ISort* isort = (ISort*)malloc(sizeof(ISort));
	if(!isort) {
		fprintf(stderr, "Couldn't malloc for ISort\n");
		return NULL;
	}

	const float x_padding = 0.68f;
	const float node_width = 1.0f;
	const float total_width = ((node_width + x_padding) * (float)INSERTION_SORT_SIZE) - x_padding;
	float x = total_width / -2.0f;
	const float y = 0.0f;
	const float z = 0.0f;
	for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
		int val = rand() % MAX_DIGITS;
		isort->nodes[i].val = val;
		isort->nodes[i].cube = GenCube(0.0f, 0.0f, 0.0f, val, 0.0f, 0.0f, 1.0f);
		isort->nodes[i].x_dest = x;
		isort->nodes[i].y_dest = y;
		isort->nodes[i].x_vel  = ISort_SetVelocity(isort->nodes[i].cube.cube_vertices[0].x, x);
		isort->nodes[i].y_vel  = 0.0f;
		isort->nodes[i].start_index = i;
		x += (1.0f + x_padding);
	}

	isort->current_state = ISORT_INITIALIZING;
	isort->previous_state = ISORT_PAUSED;
	isort->camera.x = 0.0f;
	isort->camera.z = -12.0f;

	isort->shader = LoadShaderProgram("..\\zshaders\\game_cube.vert", "..\\zshaders\\game_cube.frag");

	GLCall(glGenBuffers(1, &isort->vbo));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, isort->vbo));

	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, pos))));
	GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, rgb))));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glEnableVertexAttribArray(1));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	isort->background = GenBackgroundBuffer();
	isort->background.shader = LoadShaderProgram("..\\zshaders\\background.vert", "..\\zshaders\\background.frag");
	isort->background.texture = LoadTexture("..\\textures\\space.jpg");

	return isort;
}

INTERNAL void ISort_UpdateGeometry(ISort* isort) {
	assert(isort);

	if(isort->current_state == ISORT_PAUSED) {
		return;
	}

	for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
		ISortNode* node = &isort->nodes[i];
		GameCube* cube = &node->cube;

		int cube_vertices = sizeof(cube->cube_vertices) / sizeof(Vertex);
		for(int j = 0; j < cube_vertices; ++j) {
			cube->cube_vertices[j].x += node->x_vel;
			cube->cube_vertices[j].y += node->y_vel;
		}
		int digit_vertices = sizeof(cube->digit_vertices) / sizeof(Vertex);
		for(int j = 0; j < digit_vertices; ++j) {
			cube->digit_vertices[j].x += node->x_vel;
			cube->digit_vertices[j].y += node->y_vel;
		}
	}
}

INTERNAL void ISort_UpdateVelocitySetting(ISortNode* nodes, GameInput* input) {
	assert(nodes);
	assert(input);

	if(input->num_0.is_down) {
		units_per_second = 1.0f;
	}
	if(input->num_1.is_down) {
		units_per_second = 3.0f;
	}
	if(input->num_2.is_down) {
		units_per_second = 7.0f;
	}
	if(input->num_3.is_down) {
		units_per_second = 10.0f;
	}
	if(input->num_4.is_down) {
		units_per_second = 15.0f;
	}
	if(input->num_5.is_down) {
		units_per_second = 20.0f;
	}
	if(input->num_6.is_down) {
		units_per_second = 32.0f;
	}
	if(input->num_7.is_down) {
		units_per_second = 50.0f;
	}
	if(input->num_8.is_down) {
		units_per_second = 75.0f;
	}
	if(input->num_9.is_down) {
		units_per_second = 100.0f;
	}

	for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
		ISortNode* node = &nodes[i];
		if(node->x_vel != 0.0f) {
			node->x_vel = ISort_SetVelocity(node->cube.cube_vertices[0].x, node->x_dest);
		}
		if(node->y_vel != 0.0f) {
			node->y_vel = ISort_SetVelocity(node->cube.cube_vertices[0].y, node->y_dest);
		}
	}
}

/*  This is the state machine. It handles input, setting destinations
 * and velocities, and zeroing out velocities for each node that reaches
 * its destination
 *
 * Roughly speaking each state case checks to see if current animation
 * is finished, stops the animation by setting vel = 0.0f, and sets up 
 * the conditions for the next animation before changing state
 *
 */
INTERNAL void ISort_Update(ISort* isort, GameInput* input) {
	assert(isort);
	assert(input);

	ISort_UpdateVelocitySetting(isort->nodes, input);

	if(input->p.is_down) {
		// Pause on everything except ISORT_STATIC state. 
		// isort->previous_state is set to ISORT_PAUSED
		// in ISort_Init so we can just swap states.
		if(isort->current_state != ISORT_STATIC) {
			ISortState temp = isort->current_state;
			isort->current_state = isort->previous_state;
			isort->previous_state = temp;
		}
	}

	switch(isort->current_state) {
		case ISORT_INITIALIZING:
		{
			int num_nodes_finished = 0;
			for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
				ISortNode* node = &isort->nodes[i];
				GameCube* cube = &node->cube;
			
				if(ISort_AnimationFinished(node->x_dest, cube->cube_vertices[0].x)) {
					node->x_vel = 0.0f;
					num_nodes_finished++;
				}
			}
			
			// Once all nodes are done moving to initial positions
			// we move to static. Also set up state to begin sorting
			// by setting selected_val_index to 1. Any time ISORT_INITIALIZING
			// occurs, it is going to unsorted state so we can reliably set
			// up for sorting every time.
			if(num_nodes_finished == INSERTION_SORT_SIZE) {
				isort->selected_val_index = 1;
				isort->current_state = ISORT_STATIC;
			}
		} break;

		case ISORT_STATIC:
		{
			if(1 < INSERTION_SORT_SIZE) {
				if(input->s.is_down) {
					// array is sorted, go back to original positions
					if(isort->selected_val_index == INSERTION_SORT_SIZE) {
						// un-highlight nodes
						for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
							ISortNode* node = &isort->nodes[i];
							int num_cube_vertices = STRUCT_MEMBER_SIZE(GameCube, cube_vertices) / sizeof(Vertex);
							for(int j = 0; j < num_cube_vertices; ++j) {
								node->cube.cube_vertices[j].r = 0.0f;
								node->cube.cube_vertices[j].g = 0.0f;
								node->cube.cube_vertices[j].b = 1.0f;
							}
						}

						// UPDATE ARRAY VALUES HERE
						// sort array by start_index
						// use insertion sort obviously
						int s = 1;
						int c = 0;
						ISortNode temp = isort->nodes[s];
						while(s < INSERTION_SORT_SIZE) {
							if((0 <= c) && (temp.start_index < isort->nodes[c].start_index)) {
								isort->nodes[c + 1] = isort->nodes[c];
								--c;
							}
							else {
								isort->nodes[c + 1] = temp;
								++s;
								temp = isort->nodes[s];
								c = s - 1;
							}
						}

						// set destinations back to starting positions
						const float x_padding = 0.68f;
						const float node_width = 1.0f;
						const float total_width = ((node_width + x_padding) * 
								                  (float)INSERTION_SORT_SIZE) - x_padding;
						float x = total_width / -2.0f;
						const float y = 0.0f;
						const float z = 0.0f;
						for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
							ISortNode* node = &isort->nodes[i];
							node->x_dest = x;
							node->y_dest = y;
							node->x_vel  = ISort_SetVelocity(node->cube.cube_vertices[0].x, x);
							node->y_vel  = 0.0f;
							x += (node_width + x_padding);
						}

						isort->current_state = ISORT_INITIALIZING;
					}
					// begin sorting
					else {
						isort->nodes[isort->selected_val_index].y_dest = isort->nodes[1].cube.cube_vertices[0].y + 
							 											 y_lift_val;
						isort->nodes[isort->selected_val_index].y_vel  = ISort_SetVelocity(isort->nodes[1].cube.cube_vertices[0].y,
																                           isort->nodes[1].y_dest);

						isort->current_state = ISORT_LIFTING_SELECTED_VALUE;
					}
				}
			}
		} break;

		case ISORT_LIFTING_SELECTED_VALUE:
		{
			ISortNode* node = &isort->nodes[isort->selected_val_index];
			if(ISort_AnimationFinished(node->y_dest, node->cube.cube_vertices[0].y)) {
				isort->nodes[isort->selected_val_index].y_vel = 0.0f;
				isort->compare_val_index = isort->selected_val_index - 1;
				isort->nodes[isort->compare_val_index].x_dest = isort->nodes[isort->selected_val_index].cube.cube_vertices[0].x;
				isort->current_state = ISORT_COMPARING;
			}
		} break;

		case ISORT_COMPARING: 
		{
			ISortNode* selected_node = &isort->nodes[isort->selected_val_index];
			if(0 <= isort->compare_val_index) {
				ISortNode* compare_node = &isort->nodes[isort->compare_val_index];
				
				// highlight compare_node
				int num_cube_vertices = STRUCT_MEMBER_SIZE(GameCube, cube_vertices) / sizeof(Vertex);
				for(int j = 0; j < num_cube_vertices; ++j) {
					compare_node->cube.cube_vertices[j].r = 1.0f;
					compare_node->cube.cube_vertices[j].g = 0.0f;
					compare_node->cube.cube_vertices[j].b = 0.0f;
				}

				static int timer = 30 / (int)units_per_second;
				if(timer == 0) {
					timer = 30 / (int)units_per_second;

					if(selected_node->val < compare_node->val) {
						// leave trace for left node
						// this nodes old location will be the left
						// nodes new location if it needs to shift
						if(0 < isort->compare_val_index) {
							ISortNode* left_node = &isort->nodes[isort->compare_val_index - 1];
							left_node->x_dest = compare_node->cube.cube_vertices[0].x;
						}

						// leave trace for selected node
						// this nodes old location will also be the destination
						// of the selected node if no more right shifts occur
						selected_node->x_dest = compare_node->cube.cube_vertices[0].x;

						// give this node some right velocity and let it shift
						compare_node->x_vel  = ISort_SetVelocity(compare_node->cube.cube_vertices[0].x, compare_node->x_dest);
						isort->current_state = ISORT_SHIFTING_RIGHT;
					}
					else {
						// un-highlight compare node
						for(int j = 0; j < num_cube_vertices; ++j) {
							compare_node->cube.cube_vertices[j].r = 0.0f;
							compare_node->cube.cube_vertices[j].g = 0.0f;
							compare_node->cube.cube_vertices[j].b = 1.0f;
						}

						// set vel for selected to go left
						selected_node->x_vel = ISort_SetVelocity(selected_node->cube.cube_vertices[0].x, selected_node->x_dest);

						isort->current_state = ISORT_SHIFTING_LEFT;
					}
				}
				else {
					--timer;
				}
			}
			else {
				selected_node->x_vel = ISort_SetVelocity(selected_node->cube.cube_vertices[0].x, selected_node->x_dest);

				isort->current_state = ISORT_SHIFTING_LEFT;
			}
		} break;

		case ISORT_SHIFTING_RIGHT:
		{
			ISortNode* compare_node = &isort->nodes[isort->compare_val_index];
			if(ISort_AnimationFinished(compare_node->cube.cube_vertices[0].x,
						               compare_node->x_dest)) 
			{
				// un-highlight node when done shifting
				int num_cube_vertices = STRUCT_MEMBER_SIZE(GameCube, cube_vertices) / sizeof(Vertex);
		        for(int j = 0; j < num_cube_vertices; ++j) {
		        	compare_node->cube.cube_vertices[j].r = 0.0f;
		        	compare_node->cube.cube_vertices[j].g = 0.0f;
		        	compare_node->cube.cube_vertices[j].b = 1.0f;
		        }

				compare_node->x_vel = 0.0f;
				isort->compare_val_index -= 1;
				isort->current_state = ISORT_COMPARING;
			}
		} break;

		case ISORT_SHIFTING_LEFT:
		{
			ISortNode* selected_node = &isort->nodes[isort->selected_val_index];
			if(ISort_AnimationFinished(selected_node->cube.cube_vertices[0].x,
									   selected_node->x_dest))
			{
				selected_node->x_vel = 0.0f;
				selected_node->y_dest = 0.5f;
				selected_node->y_vel = ISort_SetVelocity(selected_node->cube.cube_vertices[0].y, selected_node->y_dest);
				isort->current_state = ISORT_SHIFTING_DOWN;
			}
		} break;

		case ISORT_SHIFTING_DOWN:
		{
			ISortNode* selected_node = &isort->nodes[isort->selected_val_index];
			if(ISort_AnimationFinished(selected_node->cube.cube_vertices[0].y,
									   selected_node->y_dest)) 
			{
				selected_node->y_vel = 0.0f;

				// UPDATE ARRAY VALS HERE
				int s = isort->selected_val_index;
				int c = isort->compare_val_index;
				ISortNode temp = isort->nodes[s];
				--s;
				while(c < s) {
					isort->nodes[s + 1] = isort->nodes[s];
					--s;
				}
				isort->nodes[s + 1] = temp;

				isort->selected_val_index += 1;

				if(isort->selected_val_index == INSERTION_SORT_SIZE) {
					// highlight all nodes to show it is sorted
					for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
						ISortNode* node = &isort->nodes[i];
						int num_cube_vertices = STRUCT_MEMBER_SIZE(GameCube, cube_vertices) / sizeof(Vertex);
						for(int j = 0; j < num_cube_vertices; ++j) {
							node->cube.cube_vertices[j].r = 1.0f;
							node->cube.cube_vertices[j].g = 1.0f;
							node->cube.cube_vertices[j].b = 0.0f;
						}
					}

					isort->current_state = ISORT_STATIC;
				}
				else {
					isort->nodes[isort->selected_val_index].y_dest = isort->nodes[isort->selected_val_index].cube.cube_vertices[0].y + 1.3f;
					isort->nodes[isort->selected_val_index].y_vel  = ISort_SetVelocity(isort->nodes[isort->selected_val_index].cube.cube_vertices[0].y,
							                                                           isort->nodes[isort->selected_val_index].y_dest);
					isort->current_state = ISORT_LIFTING_SELECTED_VALUE;
				}
			}
		} break;

		default: 
		{
			glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		} break;
	}

	ISort_UpdateGeometry(isort);
}

INTERNAL void ISort_DrawBackground(GameBackground gb, float window_width, float window_height) {

	GLCall(glBindVertexArray(gb.vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, gb.vbo));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gb.ibo));
	glBindTexture(GL_TEXTURE_2D, gb.texture);
	glUseProgram(gb.shader);

	int projection_location = glGetUniformLocation(gb.shader, "projection");
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), window_width / window_height, 0.1f, 100.0f);
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));

	glEnable(GL_DEPTH_TEST);
}

INTERNAL void ISort_Draw(ISort* isort, float window_width, float window_height) {
	assert(isort);
	assert(0.0f < window_width);
	assert(0.0f < window_height);

	ISort_DrawBackground(isort->background, window_width, window_height);

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, isort->vbo));
	GLCall(glUseProgram(isort->shader));

	unsigned int buffer_size = INSERTION_SORT_SIZE * sizeof(GameCube);
	GLCall(glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW));

	for(int i = 0; i < INSERTION_SORT_SIZE; ++i) {
		GLCall(glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(GameCube), sizeof(GameCube), 
					           &isort->nodes[i].cube));
	}
	
	GLCall(glLineWidth(4.0f));
	GLCall(glEnable(GL_DEPTH_TEST));
	//GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	int model_location = glGetUniformLocation(isort->shader, "model");
	int view_location = glGetUniformLocation(isort->shader, "view");
	int projection_location = glGetUniformLocation(isort->shader, "projection");

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(isort->camera.x, 
					                      isort->camera.y, 
					                      isort->camera.z));
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), 
			                                (float)window_width / (float)window_height, 
											0.1f, 100.0f);

	glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

	unsigned int vertices_per_cube = sizeof(GameCube) / sizeof(Vertex);
	GLCall(glDrawArrays(GL_LINES, 0, INSERTION_SORT_SIZE * VERTICES_PER_CUBE));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
