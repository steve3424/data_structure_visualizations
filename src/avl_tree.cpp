static int global_num = 100;
// TODO: delete above

#include "engine.h"
#include "windows.h"

// TODO: need to tune this
#define AVL_THRESHOLD 0.001f


// units is a unit cube 1.0f
static float avl_tree_units_per_second = 8.0f;
static float const avl_tree_frames_per_second = 60.0f;

typedef enum {
	AVLTREE_STATIC,
	AVLTREE_PAUSED
} AVLTreeState;

typedef struct AVLNode {
	// data
	struct AVLNode* parent;
	struct AVLNode* left;
	struct AVLNode* right;
	int val;
	int height;

	// geometry
	GameCube cube;
	float x_dest;
	float x_vel;
	float y_dest;
	float y_vel;
} AVLNode;

typedef struct {
	// data
	int size;
	AVLNode* root;

	// opengl
	GameCamera camera;
	unsigned int vao;
	unsigned int vbo;
	unsigned int shader;
	GameBackground background;

	// state machine stuff
	AVLTreeState current_state;
	AVLTreeState previous_state;
} AVLTree;


// AVL Tree data functions
static int AVLTree_GetHeight(const AVLNode* node) {
	return (node == NULL) ? -1 : node->height;
}

static void AVLTree_UpdateHeight(AVLNode* node) {
	assert(node);

	int hl = AVLTree_GetHeight(node->left);
	int hr = AVLTree_GetHeight(node->right);
	node->height = (hl > hr) ? (hl + 1) : (hr + 1);
}

static int AVLTree_GetBalance(const AVLNode* node) {
	assert(node);
	int hl = AVLTree_GetHeight(node->left);
	int hr = AVLTree_GetHeight(node->right);
	return hl - hr;
}

static AVLNode* AVLTree_RightRotate(AVLNode* const node) {
	assert(node);

	AVLNode* const parent   = node->parent;
	AVLNode* const left     = node->left;
	AVLNode* const new_left = left->right;

	// shift left node to this node's position
	node->parent = left;
	left->right = node;

	// attach left's right subtree to this node's left
	node->left = new_left;
	if(new_left != NULL) {
		new_left->parent = node;
	}

	// attach parent to new node
	left->parent = parent;
	if(parent != NULL) {
		if(parent->left == node) {
			parent->left = left;
		}
		else {
			parent->right = left;
		}
	}

	// NOTE: must be in this order
	//       because height of left
	//       relies on new height of node
	AVLTree_UpdateHeight(node);
	AVLTree_UpdateHeight(left);

	// return left node which is taking the place of the node
	// passed in
	return left;
}

static AVLNode* AVLTree_LeftRotate(AVLNode* const node) {
	assert(node);

	AVLNode* const parent    = node->parent;
	AVLNode* const right     = node->right;
	AVLNode* const new_right = right->left;

	// shift right node to this node's position
	node->parent = right;
	right->left = node;

	// attach right's left subtree to this node's right
	node->right = new_right;
	if(new_right) {
		new_right->parent = node;
	}

	// attach parent to new node
	right->parent = parent;
	if(parent != NULL) {
		if(parent->left == node) {
			parent->left = right;
		}
		else {
			parent->right = right;
		}
	}

	// NOTE: must be in this order
	//       because height of right
	//       relies on new height of node
	AVLTree_UpdateHeight(node);
	AVLTree_UpdateHeight(right);

	// return right node which is taking the place of the node
	// passed in
	return right;
}

void AVLTree_Insert(AVLTree *const tree, const int val) {
	assert(tree);
	assert(-1 < val);

	// normal BST insert
	AVLNode* parent_node = NULL;
	AVLNode* current_node = tree->root;
	while(current_node != NULL) {
		parent_node = current_node;
		if(val < current_node->val) {
			current_node = current_node->left;
		}
		else if(val > current_node->val) {
			current_node = current_node->right;
		}
		else {
			return;
		}
	}

	current_node = (AVLNode*)calloc(1, sizeof(AVLNode));
	current_node->parent = parent_node;
	current_node->val = val;
	tree->size++;
	if(parent_node == NULL) {
		tree->root = current_node;
	}
	else {
		// NOTE: for generic keys, key comparison might
		//       be slow so we can avoid that here if necessary
		if(val < parent_node->val) {
			parent_node->left = current_node;
		}
		else {
			parent_node->right = current_node;
		}
	}

	// go back up tree
	// adjust heights
	// check for imbalance
	// rotate if necessary
	current_node = current_node->parent;
	while(current_node != NULL) {
		AVLTree_UpdateHeight(current_node);
		int balance = AVLTree_GetBalance(current_node);

		if((balance > 1) && (val < current_node->left->val)) {
			current_node = AVLTree_RightRotate(current_node);
		}
		else if((balance < -1) && (val > current_node->right->val)) {
			current_node = AVLTree_LeftRotate(current_node);
		}
		else if((balance > 1) && (val > current_node->left->val)) {
			AVLTree_LeftRotate(current_node->left);
			current_node = AVLTree_RightRotate(current_node);
		}
		else if((balance < -1) && (val < current_node->right->val)) {
			AVLTree_RightRotate(current_node->right);
			current_node = AVLTree_LeftRotate(current_node);
		}

		if(current_node->parent == NULL) {
			tree->root = current_node;
		}

		current_node = current_node->parent;
	}
}

// Geometry stuff

static void AVLTree_UpdateGeometry(AVLTree* avl_tree) {
	assert(avl_tree);

	if(avl_tree->current_state == AVLTREE_PAUSED) {
		return;
	}

	// BFS
	int push = 0;
	int pop = 0;
	int size = 0;
	AVLNode* nodes[MAX_DIGITS] = {NULL};
	nodes[push] = avl_tree->root;
	++push;
	++size;
	while(size > 0) {
		int nodes_at_this_level = size;
		for(int i = 0; i < nodes_at_this_level; ++i) {
			AVLNode* node = nodes[pop];
			GameCube* cube = &node->cube;
			pop = (pop + 1) % MAX_DIGITS;
			--size;


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
	
			// draw line to parent
			// lines will go from center top of child
			// to center bottom of parent
			// TODO: Change this if I ever #define node width
			// TODO: This probably shouldn't be here. This only needs to be
			//       done during rotations and not during static drawing.
			if(node->parent) {
				AVLNode* parent = node->parent;
				if(node == parent->left) {
					node->cube.line_vertices[0] = node->cube.cube_vertices[1];
					node->cube.line_vertices[1] = parent->cube.cube_vertices[5];

					node->cube.line_vertices[0].x -= 0.5f;
					node->cube.line_vertices[0].z -= 0.5f;
					node->cube.line_vertices[0].r = 0.0f;
					node->cube.line_vertices[0].g = 1.0f;
					node->cube.line_vertices[0].b = 0.0f;

					node->cube.line_vertices[1].x += 0.5f;
					node->cube.line_vertices[1].z -= 0.5f;
					node->cube.line_vertices[1].r = 0.0f;
					node->cube.line_vertices[1].g = 1.0f;
					node->cube.line_vertices[1].b = 0.0f;
				}
				else {
					// line will go from top left of this cube
					// to bottom right of parent
					node->cube.line_vertices[0] = node->cube.cube_vertices[0];
					node->cube.line_vertices[1] = parent->cube.cube_vertices[3];

					node->cube.line_vertices[0].x += 0.5f;
					node->cube.line_vertices[0].z -= 0.5f;
					node->cube.line_vertices[0].r = 0.0f;
					node->cube.line_vertices[0].g = 1.0f;
					node->cube.line_vertices[0].b = 0.0f;

					node->cube.line_vertices[1].x -= 0.5f;
					node->cube.line_vertices[1].z -= 0.5f;
					node->cube.line_vertices[1].r = 0.0f;
					node->cube.line_vertices[1].g = 1.0f;
					node->cube.line_vertices[1].b = 0.0f;
				}
			}

			if(node->left) {
				nodes[push] = node->left;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}

			if(node->right) {
				nodes[push] = node->right;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}
		}
	}
}

// TODO: this should probably just take Node as argument
// TODO: I think if the animation is finished, it should set the location to destination.
//       This might mitigate any floating point inaccuracies that could creep in leaving
//       the location somewhere in the threshold interval
static inline bool AVLTree_AnimationFinished(float const location, float const destination) {
	float diff = destination - location;
	if(diff < 0.0f) {
		diff *= -1.0f;
	}

	return diff <= AVL_THRESHOLD;
}

static void AVLTree_Update(AVLTree* avl_tree) {
	assert(avl_tree);

	switch(avl_tree->current_state) {
		case AVLTREE_STATIC:
		{
			// BFS
			int push = 0;
			int pop = 0;
			int size = 0;
			AVLNode* nodes[MAX_DIGITS] = {NULL};
			nodes[push] = avl_tree->root;
			++push;
			++size;
			int num_nodes_finished = 0;
			while(size > 0) {
				int nodes_at_this_level = size;
				for(int i = 0; i < nodes_at_this_level; ++i) {
					AVLNode* node = nodes[pop];
					GameCube* cube = &node->cube;
					pop = (pop + 1) % MAX_DIGITS;
					--size;
					
					bool x_animation_finished = AVLTree_AnimationFinished(cube->cube_vertices[0].x, node->x_dest);
					bool y_animation_finished = AVLTree_AnimationFinished(cube->cube_vertices[0].y, node->y_dest);
					if(x_animation_finished) {
						node->x_vel = 0.0f;
					}
					if(y_animation_finished) {
						node->y_vel = 0.0f;
					}
					if(x_animation_finished && y_animation_finished) {
						num_nodes_finished++;
					}
			
					if(node->left) {
						nodes[push] = node->left;
						push = (push + 1) % MAX_DIGITS;
						++size;
					}

					if(node->right) {
						nodes[push] = node->right;
						push = (push + 1) % MAX_DIGITS;
						++size;
					}
				}
			}

		} break;
	}

	AVLTree_UpdateGeometry(avl_tree);
}

inline float AVLTree_SetVelocity(float const location, float const destination, float const scale) {
	float dist = destination - location;
	float units_per_frame = (avl_tree_units_per_second * scale) / avl_tree_frames_per_second;
	float frames_to_reach_dest = dist / units_per_frame;
	if(frames_to_reach_dest < 0.0f) {
		frames_to_reach_dest *= -1.0f;
	}
	int new_frames_to_reach_dest = (int)(frames_to_reach_dest + 1.0f);
	return dist / (float)new_frames_to_reach_dest;
}

static AVLTree* AVLTree_Init() {
	AVLTree* avl_tree = (AVLTree*)calloc(1, sizeof(AVLTree));
	if(!avl_tree) {
		fprintf(stderr, "Couldn't malloc for AVLTree\n");
		return NULL;
	}

	// Initialize entire tree since the tree will rotate
	// and positions will change.
	for(int i = 0; i < global_num; ++i) {
		// TODO: change
		//int val = rand() % MAX_DIGITS;
		int val = i;
		AVLTree_Insert(avl_tree, val);
	}

	// Set up spacing, width, ...
	int bottom_level_width = 1;
	for(int i = 0; i < avl_tree->root->height; ++i) {
		bottom_level_width *= 2;
	}
	const float node_width = 1.0f; // this is fixed based on the model sent to the GPU
	const float node_margin = 0.75f; // space between the nodes at the bottom level
	const float max_width = bottom_level_width * (node_width + node_margin) - node_margin; // subtract one node_margin for the far right node

	float split = 2.0f;
	float x_width = (max_width / split);
	const float x_start = x_width * -1.0f;
	float y_pos = 0.0f;
	float y_spacing = 4.0f; // space between successive levels of the tree

	// BFS
	int push = 0;
	int pop = 0;
	int size = 0;
	uint64_t node_indices[MAX_DIGITS] = {0};
	AVLNode* nodes[MAX_DIGITS] = {NULL};
	nodes[push] = avl_tree->root;
	++push;
	++size;
	while(size > 0) {
		int nodes_at_this_level = size;
		for(int i = 0; i < nodes_at_this_level; ++i) {
			AVLNode* current_node = nodes[pop];
			uint64_t current_index = node_indices[pop];
			pop = (pop + 1) % MAX_DIGITS;
			--size;
			
			// map node indices to odd #'s
			uint64_t split_index = (current_index * 2) + 1;
			// x_pos is the center of the node
			float x_pos = x_start + ((float)split_index * x_width);
			current_node->cube = GenCube(0.0f, 0.0f, 0.0f, current_node->val, 0.0f, 0.0f, 1.0f);
			// destination is based on the top left corner or the first
			// vertex of the node
			current_node->x_dest = x_pos - 0.5f;
			current_node->y_dest = y_pos + 0.5f;
			current_node->x_vel = AVLTree_SetVelocity(current_node->cube.cube_vertices[0].x, current_node->x_dest, 1.0f); 
			current_node->y_vel = AVLTree_SetVelocity(current_node->cube.cube_vertices[0].y, current_node->y_dest, 1.0f); 

			if(current_node != avl_tree->root) {
				float x_over_y = x_pos / y_pos;
				float y_over_x = y_pos / x_pos;
				if(x_over_y < 0.0f) {
					x_over_y *= -1.0f;
					y_over_x *= -1.0f;
				}

				if(x_over_y < y_over_x) {
					current_node->x_vel = AVLTree_SetVelocity(current_node->cube.cube_vertices[0].x, current_node->x_dest, x_over_y);
				}
				else {
					current_node->y_vel = AVLTree_SetVelocity(current_node->cube.cube_vertices[0].y, current_node->y_dest, y_over_x);
				}
			}

			if(current_node->left) {
				nodes[push] = current_node->left;
				node_indices[push] = (2 * current_index);
				push = (push + 1) % MAX_DIGITS;
				++size;
			}

			if(current_node->right) {
				nodes[push] = current_node->right;
				node_indices[push] = (2 * current_index) + 1;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}
		}

		split *= 2.0f;
		x_width = (max_width / split);
		y_pos -= y_spacing;
	}

	avl_tree->current_state = AVLTREE_STATIC;
	// TODO: Adjust camera
	avl_tree->camera.x = 0.0f;
	avl_tree->camera.y = 6.0f;
	avl_tree->camera.z = -50.0f;

	// Initialize opengl stuff
	avl_tree->shader = LoadShaderProgram("..\\zshaders\\game_cube.vert", "..\\zshaders\\game_cube.frag");

	GLCall(glGenVertexArrays(1, &avl_tree->vao));
	GLCall(glBindVertexArray(avl_tree->vao));
	GLCall(glGenBuffers(1, &avl_tree->vbo));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, avl_tree->vbo));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, pos))));
	GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, rgb))));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glEnableVertexAttribArray(1));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));

	avl_tree->background = GenBackgroundBuffer();
	avl_tree->background.shader = LoadShaderProgram("..\\zshaders\\background.vert", "..\\zshaders\\background.frag");
	avl_tree->background.texture = LoadTexture("..\\textures\\space.jpg");

	return avl_tree;
}

INTERNAL void AVLTree_DrawBackground(GameBackground gb, float window_width, float window_height) {
	GLCall(glBindVertexArray(gb.vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, gb.vbo));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gb.ibo));
	glBindTexture(GL_TEXTURE_2D, gb.texture);
	glUseProgram(gb.shader);

	int projection_location = glGetUniformLocation(gb.shader, "projection");
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), window_width / window_height, 0.1f, 100.0f);
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

	glDisable(GL_DEPTH_TEST);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));
}

static void AVLTree_Draw(AVLTree* avl_tree, float window_width, float window_height) {
	assert(avl_tree);
	assert(0.0f < window_width);
	assert(0.0f < window_height);

	GLCall(glLineWidth(4.0f));
	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	AVLTree_DrawBackground(avl_tree->background, window_width, window_height);

	GLCall(glBindVertexArray(avl_tree->vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, avl_tree->vbo));
	GLCall(glUseProgram(avl_tree->shader));

	unsigned int buffer_size = avl_tree->size * sizeof(GameCube);
	GLCall(glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW));

	// BFS
	int push = 0;
	int pop = 0;
	int size = 0;
	AVLNode* nodes[MAX_DIGITS] = {NULL};
	nodes[push] = avl_tree->root;
	++push;
	++size;
	unsigned int buffer_write_index = 0;
	while(size > 0) {
		int nodes_at_this_level = size;
		for(int i = 0; i < nodes_at_this_level; ++i) {
			AVLNode* current_node = nodes[pop];
			pop = (pop + 1) % MAX_DIGITS;
			--size;

			GLCall(glBufferSubData(GL_ARRAY_BUFFER, buffer_write_index, sizeof(GameCube), 
						           &current_node->cube));
			buffer_write_index += sizeof(GameCube);

			if(current_node->left) {
				nodes[push] = current_node->left;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}

			if(current_node->right) {
				nodes[push] = current_node->right;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}
		}
	}

	int model_location = glGetUniformLocation(avl_tree->shader, "model");
	int view_location = glGetUniformLocation(avl_tree->shader, "view");
	int projection_location = glGetUniformLocation(avl_tree->shader, "projection");

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(avl_tree->camera.x, 
					                      avl_tree->camera.y, 
					                      avl_tree->camera.z));
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), 
			                                window_width / window_height, 
											0.1f, 100.0f);

	glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

	unsigned int vertices_per_cube = sizeof(GameCube) / sizeof(Vertex);
	GLCall(glDrawArrays(GL_LINES, 0, avl_tree->size * VERTICES_PER_CUBE));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));
}
