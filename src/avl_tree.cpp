static int         global_avl_tree_init_size = 7;
static float       global_avl_tree_units_per_second = 5.0f; // units is a unit cube 1.0f
static float const global_avl_tree_frames_per_second = 60.0f;
static int         global_avl_tree_timer_reset = 240 / (int)global_avl_tree_units_per_second;
static float const global_node_width = 1.0f; // this is fixed based on the model sent to the GPU
static float const global_node_margin = 0.75f; // space between the nodes at the bottom level
static float const global_y_spacing = 3.0f; // space between successive levels of the tree
static float const global_y_insert_node_start = 1.3f; // where insert node begins, this should be 
                                                      // subtracted on final insert

#include "engine.h"
#include "windows.h"

#define AVL_THRESHOLD 0.001f

typedef enum {
	AVLTREE_INITIALIZING,
	AVLTREE_STATIC,
	AVLTREE_INSERT_NODE_COMPARE,
	AVLTREE_INSERT_NODE_MOVING_TO_NEXT_COMPARE,
	AVLTREE_INSERT_NODE_DELETE,
	AVLTREE_INSERT_NODE_ADD,
	AVLTREE_UPDATE_HEIGHTS,
	AVLTREE_ROTATING,
	AVLTREE_LEFT_RIGHT_ROTATE,
	AVLTREE_RIGHT_LEFT_ROTATE,
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
	AVLNode* detached_node;
	AVLNode* inserted_node;
	AVLNode* compare_node;

	// These are used for left_right rotations and
	// right_left rotations. It makes the state 
	// machine code a bit simpler than it otherwise
	// would be.
	bool left_rotate;
	bool right_rotate;
} AVLTree;


/*********************************************
 * AVLTree data functions					 *
 *********************************************/
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

/* Streams nodes from a tree in BFS fashion, e.g.:
 
AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
while(bfs_node.node) {                          
	AVLNode* node = bfs_node.node;                 
	bfs_node = AVLTree_BFS(avl_tree);          
}                                               
 *
 */
typedef struct {
	AVLNode* node;
	int level;
	uint64_t level_index;
} AVLTreeBFSNode;

// TODO: Restart if called w/ new tree.
static AVLTreeBFSNode AVLTree_BFS(AVLTree* avl_tree) {
	AVLTreeBFSNode result;
	result.node = NULL;
	result.level = -1;
	result.level_index = 0;

	if(!avl_tree || !avl_tree->root) {
		return result;
	}

	static AVLTree* tree = NULL;
	static AVLNode* node_queue[MAX_DIGITS];
	static uint64_t level_indices[MAX_DIGITS];
	static int push;
	static int pop;
	static int size;
	static int level;
	static int nodes_at_level;
	static int i;
	if(tree == NULL) {
		push = 0;
		pop = 0;
		size = 0;
		level = 0;
		i = 0;
		tree = avl_tree;
		node_queue[push] = avl_tree->root;
		level_indices[push] = 0;
		++push;
		++size;

		// NOTE: This needs to come after ++size
		nodes_at_level = size;
	}

	while(size > 0) {
		while(i++ < nodes_at_level) {
			AVLNode* node = node_queue[pop];
			uint64_t level_index = level_indices[pop];
			pop = (pop + 1) % MAX_DIGITS;
			--size;

			if(node->left) {
				node_queue[push] = node->left;
				level_indices[push] = 2 * level_index;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}

			if(node->right) {
				node_queue[push] = node->right;
				level_indices[push] = (2 * level_index) + 1;
				push = (push + 1) % MAX_DIGITS;
				++size;
			}

			result.node = node;
			result.level = level;
			result.level_index = level_index;
			return result;
		}

		i = 0;
		nodes_at_level = size;
		++level;
	}

	tree = NULL;
	return result;
}

/*********************************************
 * State machine helper functions			 *
 *********************************************/
static float AVLTree_GetNodeSplitWidth(const AVLNode* node, const int tree_height) {
	assert(node);

	// get node level in tree
	int level = 0;
	while(node->parent) {
		++level;
		node = node->parent;
	}
	// get split of level below current
	float split = exp2f((float)(level + 2));
	const int   bottom_level_width = 1 << tree_height;
	const float max_tree_width = (float)bottom_level_width * 
		                         (global_node_width + global_node_margin) - 
								 global_node_margin; // subtract one node_margin for the far right node
	return max_tree_width / split;
}

static inline bool AVLTree_AnimationFinished(AVLNode* node) {
	float x_dist = fabs(node->x_dest - node->cube.cube_vertices[0].x);
	float y_dist = fabs(node->y_dest - node->cube.cube_vertices[0].y);
	bool x_finished = x_dist <= AVL_THRESHOLD;
	bool y_finished = y_dist <= AVL_THRESHOLD;
	if(x_finished) {
		node->x_vel = 0.0f;
	}
	if(y_finished) {
		node->y_vel = 0.0f;
	}

	return (node->x_vel == 0.0f) && (node->y_vel == 0.0f);
}

// Automatically syncs x_vel and y_vel so they land at the same time.
inline void AVLTree_SetVelocity(AVLNode* node) {
	float x_dist = node->x_dest - node->cube.cube_vertices[0].x;
	float y_dist = node->y_dest - node->cube.cube_vertices[0].y;

	float units_per_frame = global_avl_tree_units_per_second / global_avl_tree_frames_per_second;
	float x_frames_to_reach_dest = fabs(x_dist / units_per_frame);
	float y_frames_to_reach_dest = fabs(y_dist / units_per_frame);
	int frames_to_reach_dest;
	if(x_frames_to_reach_dest < y_frames_to_reach_dest) {
		frames_to_reach_dest = (int)(y_frames_to_reach_dest + 1);
	}
	else {
		frames_to_reach_dest = (int)(x_frames_to_reach_dest + 1);
	}
	node->x_vel = x_dist / (float)frames_to_reach_dest;
	node->y_vel = y_dist / (float)frames_to_reach_dest;
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

static void AVLTree_UpdateGeometry(AVLTree* avl_tree) {
	assert(avl_tree);

	if(avl_tree->current_state == AVLTREE_PAUSED) {
		return;
	}

	AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
	while(bfs_node.node) {
		AVLNode* node = bfs_node.node;
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
				node->cube.line_vertices[0].r = 1.0f;
				node->cube.line_vertices[0].g = 1.0f;
				node->cube.line_vertices[0].b = 153.0f / 255.0f;

				node->cube.line_vertices[1].x += 0.5f;
				node->cube.line_vertices[1].z -= 0.5f;
				node->cube.line_vertices[1].r = 1.0f;
				node->cube.line_vertices[1].g = 1.0f;
				node->cube.line_vertices[1].b = 153.0f / 255.0f;
			}
			else {
				// line will go from top left of this cube
				// to bottom right of parent
				node->cube.line_vertices[0] = node->cube.cube_vertices[0];
				node->cube.line_vertices[1] = parent->cube.cube_vertices[3];

				node->cube.line_vertices[0].x += 0.5f;
				node->cube.line_vertices[0].z -= 0.5f;
				node->cube.line_vertices[0].r = 1.0f;
				node->cube.line_vertices[0].g = 1.0f;
				node->cube.line_vertices[0].b = 153.0f / 255.0f;

				node->cube.line_vertices[1].x -= 0.5f;
				node->cube.line_vertices[1].z -= 0.5f;
				node->cube.line_vertices[1].r = 1.0f;
				node->cube.line_vertices[1].g = 1.0f;
				node->cube.line_vertices[1].b = 153.0f / 255.0f;
			}
		}
		else {
			// clear parent for root node
			node->cube.line_vertices[0].x = 0.0f;
			node->cube.line_vertices[0].y = 0.0f;
			node->cube.line_vertices[0].z = 0.0f;
			node->cube.line_vertices[1].x = 0.0f;
			node->cube.line_vertices[1].y = 0.0f;
			node->cube.line_vertices[1].z = 0.0f;
		}

		bfs_node = AVLTree_BFS(avl_tree);
	}
	
	if(avl_tree->detached_node) {
		AVLNode* node = avl_tree->detached_node;
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

static void AVLTree_SetEntireTreeDest(AVLTree* avl_tree) {
	assert(avl_tree);

	const int   bottom_level_width = 1 << avl_tree->root->height;
	const float max_tree_width = bottom_level_width * (global_node_width + global_node_margin) - global_node_margin; // subtract one node_margin for the far right node
	const float x_start = (max_tree_width / 2.0f) * -1.0f;
	const float y_start = 0.0f;

	AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
	while(bfs_node.node) {
		AVLNode* node = bfs_node.node;

		float split = exp2f((float)(bfs_node.level + 1));
		float x_width = (max_tree_width / split);
		// map node indices to odd #'s
		uint64_t split_index = (bfs_node.level_index * 2) + 1;
		// x_pos, y_pos is the center of the node
		float x_pos = x_start + ((float)split_index * x_width);
		float y_pos = y_start - ((float)bfs_node.level * global_y_spacing);

		// destination is based on top left front corner of cube
		node->x_dest = x_pos - 0.5f;
		node->y_dest = y_pos + 0.5f;
		AVLTree_SetVelocity(node);

		bfs_node = AVLTree_BFS(avl_tree);
	}
}

/*********************************************
 * Public functions                          *
 *********************************************/
void AVLTree_Draw(AVLTree* avl_tree, float window_width, float window_height) {
	assert(avl_tree);
	assert(0.0f < window_width);
	assert(0.0f < window_height);

	GLCall(glLineWidth(4.0f));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	AVLTree_DrawBackground(avl_tree->background, window_width, window_height);

	// NOTE: I should enable depth test here, but I like the way it
	//       puts the numbers in front of the cube when viewing from
	//       the side to make it more readable.

	GLCall(glBindVertexArray(avl_tree->vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, avl_tree->vbo));
	GLCall(glUseProgram(avl_tree->shader));

	// NOTE: One for inserting node possibly
	unsigned int buffer_size = (MAX_DIGITS + 1) * sizeof(GameCube);
	GLCall(glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW));
	unsigned int buffer_write_index = 0;
 	AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
 	while(bfs_node.node) {
		AVLNode* node = bfs_node.node;

		GLCall(glBufferSubData(GL_ARRAY_BUFFER, buffer_write_index, 
					           sizeof(GameCube), &node->cube));
		buffer_write_index += sizeof(GameCube);

 	   	bfs_node = AVLTree_BFS(avl_tree);
 	}

	if(avl_tree->detached_node) {
		GLCall(glBufferSubData(GL_ARRAY_BUFFER, buffer_write_index, 
					           sizeof(GameCube), &avl_tree->detached_node->cube));
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
	// NOTE: One for inserting node possibly
	unsigned int num_cubes = avl_tree->detached_node ?
		                     avl_tree->size + 1       :
							 avl_tree->size;
	GLCall(glDrawArrays(GL_LINES, 0, num_cubes * VERTICES_PER_CUBE));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));
}

void AVLTree_Update(AVLTree* avl_tree, GameInput* input) {
	assert(avl_tree);
	assert(input);

	if(input->p.is_down) {
		if(avl_tree->current_state != AVLTREE_STATIC) {
			AVLTreeState temp = avl_tree->current_state;
			avl_tree->current_state = avl_tree->previous_state;
			avl_tree->previous_state = temp;
		}
	}

	switch(avl_tree->current_state) {
		case AVLTREE_INITIALIZING:
		{
			int num_nodes_finished = 0;

			AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
			while(bfs_node.node) {                          
				AVLNode* node = bfs_node.node;                 
				GameCube* cube = &node->cube;

				if(AVLTree_AnimationFinished(node)) {
					num_nodes_finished++;
				}
			
				bfs_node = AVLTree_BFS(avl_tree);          
			}                                               

			if(num_nodes_finished == avl_tree->size) {
				avl_tree->current_state = AVLTREE_STATIC;
			}
		} break;

		case AVLTREE_STATIC: 
		{
			if(input->a.is_down) {
				int val = rand() % MAX_DIGITS;
				AVLNode* node = (AVLNode*)calloc(1, sizeof(AVLNode));
				node->val = val;
				if(!avl_tree->root) {
					node->cube = GenCube(0.0f, 0.0f, 0.0f, val, 0.0f, 0.0f, 1.0f);
					avl_tree->root = node;
					avl_tree->size = 1;
				}
				else {
					node->cube = GenCube(0.0f, global_y_insert_node_start, 0.0f, val, 1.0f, 140.0f / 255.0f, 0.0f);
					avl_tree->detached_node = node;
					avl_tree->compare_node = avl_tree->root;
					avl_tree->current_state = AVLTREE_INSERT_NODE_COMPARE;
				}
			}
		} break;

		case AVLTREE_INSERT_NODE_COMPARE:
		{
			if(avl_tree->detached_node->val == 
			   avl_tree->compare_node->val) 
			{
				AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
				while(bfs_node.node) {
					GameCube_SetColor(&bfs_node.node->cube, 1.0f, 0.0f, 0.0f);
					bfs_node = AVLTree_BFS(avl_tree);
				}
				GameCube_SetColor(&avl_tree->detached_node->cube, 1.0f, 0.0f, 0.0f);
				avl_tree->current_state = AVLTREE_INSERT_NODE_DELETE;
			}
			else {
				GameCube_SetColor(&avl_tree->compare_node->cube, 1.0f, 140.0f / 255.0f, 0.0f);

				static int timer = global_avl_tree_timer_reset;
				if(timer == 0) {
					timer = global_avl_tree_timer_reset;

					GameCube_SetColor(&avl_tree->compare_node->cube, 0.0f, 0.0f, 1.0f);

					avl_tree->detached_node->y_dest = avl_tree->detached_node->cube.cube_vertices[0].y - global_y_spacing;
					float x_spacing = AVLTree_GetNodeSplitWidth(avl_tree->compare_node, avl_tree->root->height);

					if(avl_tree->detached_node->val < 
					   avl_tree->compare_node->val) 
					{
						// BASE CASE: insert into tree
						if(avl_tree->compare_node->left == NULL) {
							avl_tree->detached_node->y_dest -= global_y_insert_node_start;
							avl_tree->detached_node->x_dest = avl_tree->detached_node->cube.cube_vertices[0].x - x_spacing;
							AVLTree_SetVelocity(avl_tree->detached_node);
							avl_tree->current_state = AVLTREE_INSERT_NODE_ADD;
						}
						else {
							avl_tree->detached_node->x_dest = avl_tree->detached_node->cube.cube_vertices[0].x - x_spacing;
							AVLTree_SetVelocity(avl_tree->detached_node);

							avl_tree->compare_node = avl_tree->compare_node->left;

							avl_tree->current_state = AVLTREE_INSERT_NODE_MOVING_TO_NEXT_COMPARE;
						}
					}
					else if(avl_tree->detached_node->val > 
					        avl_tree->compare_node->val) 
					{
						// BASE CASE: insert into tree
						if(avl_tree->compare_node->right == NULL) {
							avl_tree->detached_node->y_dest -= global_y_insert_node_start;
							avl_tree->detached_node->x_dest = avl_tree->detached_node->cube.cube_vertices[0].x + x_spacing;
							AVLTree_SetVelocity(avl_tree->detached_node);
							avl_tree->current_state = AVLTREE_INSERT_NODE_ADD;
						}
						else {
							avl_tree->detached_node->x_dest = avl_tree->detached_node->cube.cube_vertices[0].x + x_spacing;
							AVLTree_SetVelocity(avl_tree->detached_node);

							avl_tree->compare_node = avl_tree->compare_node->right;

							avl_tree->current_state = AVLTREE_INSERT_NODE_MOVING_TO_NEXT_COMPARE;
						}
					}
				}
				else {
					--timer;
				}
			}
		} break;

		case AVLTREE_INSERT_NODE_MOVING_TO_NEXT_COMPARE: 
		{
			if(AVLTree_AnimationFinished(avl_tree->detached_node)) {
				avl_tree->current_state = AVLTREE_INSERT_NODE_COMPARE;
			}
		} break;

		case AVLTREE_INSERT_NODE_DELETE: 
		{
			static int timer = global_avl_tree_timer_reset;
			if(timer == 0) {
				timer = global_avl_tree_timer_reset;

				free(avl_tree->detached_node);
				avl_tree->detached_node = NULL;
				avl_tree->compare_node = NULL;
				AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
				while(bfs_node.node) {
					GameCube_SetColor(&bfs_node.node->cube, 0.0f, 0.0f, 1.0f);
					bfs_node = AVLTree_BFS(avl_tree);
				}
				avl_tree->current_state = AVLTREE_STATIC;
			}
			else {
				--timer;
			}
		} break;

		case AVLTREE_INSERT_NODE_ADD:
		{
			if(AVLTree_AnimationFinished(avl_tree->detached_node)) {
				GameCube_SetColor(&avl_tree->detached_node->cube, 0.0f, 0.0f, 1.0f);

				avl_tree->detached_node->parent = avl_tree->compare_node;

				// Check to see if it is left or right because
				// I don't track where it came from.
				if(avl_tree->detached_node->val < 
				   avl_tree->compare_node->val) 
				{
					avl_tree->compare_node->left = avl_tree->detached_node;
				}
				else {
					avl_tree->compare_node->right = avl_tree->detached_node;
				}

				avl_tree->size++;
				avl_tree->inserted_node = avl_tree->detached_node;
				avl_tree->detached_node = NULL;

				avl_tree->current_state = AVLTREE_UPDATE_HEIGHTS;
			}
		} break;

		case AVLTREE_UPDATE_HEIGHTS:
		{
			if(avl_tree->compare_node) {
				AVLTree_UpdateHeight(avl_tree->compare_node);
				int balance = AVLTree_GetBalance(avl_tree->compare_node);

				if(((balance > 1) && 
				   (avl_tree->inserted_node->val < avl_tree->compare_node->left->val)) ||
					avl_tree->right_rotate) 
				{
					avl_tree->right_rotate = false;
					avl_tree->compare_node = AVLTree_RightRotate(avl_tree->compare_node);
				}
				else if(((balance < -1) && 
						(avl_tree->inserted_node->val > avl_tree->compare_node->right->val)) ||
						avl_tree->left_rotate)
				{
					avl_tree->left_rotate = false;
					avl_tree->compare_node = AVLTree_LeftRotate(avl_tree->compare_node);
				}
				else if((balance > 1) && 
					    (avl_tree->inserted_node->val > avl_tree->compare_node->left->val)) 
				{
					AVLTree_LeftRotate(avl_tree->compare_node->left);
					AVLTree_SetEntireTreeDest(avl_tree);
					avl_tree->right_rotate = true;
					avl_tree->current_state = AVLTREE_ROTATING;
					break;
				}
				else if((balance < -1) && 
					    (avl_tree->inserted_node->val < avl_tree->compare_node->right->val)) 
				{
					AVLTree_RightRotate(avl_tree->compare_node->right);
					AVLTree_SetEntireTreeDest(avl_tree);
					avl_tree->left_rotate = true;
					avl_tree->current_state = AVLTREE_ROTATING;
					break;
				}

				if(avl_tree->compare_node->parent == NULL) {
					avl_tree->root = avl_tree->compare_node;
				}

				AVLTree_SetEntireTreeDest(avl_tree);
				avl_tree->compare_node = avl_tree->compare_node->parent;
				avl_tree->current_state = AVLTREE_ROTATING;
			}
			else {
				avl_tree->current_state = AVLTREE_STATIC;
			}
		} break;

		case AVLTREE_ROTATING:
		{
			int num_nodes_finished = 0;

			AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
			while(bfs_node.node) {                          
				AVLNode* node = bfs_node.node;                 
				GameCube* cube = &node->cube;

				if(AVLTree_AnimationFinished(node)) {
					num_nodes_finished++;
				}
			
				bfs_node = AVLTree_BFS(avl_tree);          
			}                                               

			if(num_nodes_finished == avl_tree->size) {
				avl_tree->current_state = AVLTREE_UPDATE_HEIGHTS;
			}
		} break;
	}

	AVLTree_UpdateGeometry(avl_tree);
}

AVLTree* AVLTree_Init() {
	AVLTree* avl_tree = (AVLTree*)calloc(1, sizeof(AVLTree));
	if(!avl_tree) {
		fprintf(stderr, "Couldn't malloc for AVLTree\n");
		return NULL;
	}

	// Initialize entire tree since the tree will rotate
	// and positions will change.
	for(int i = 0; i < global_avl_tree_init_size; ++i) {
		int val = rand() % MAX_DIGITS;
		AVLTree_Insert(avl_tree, val);
	}

	if(avl_tree->root) {
		// Set up spacing, width, ..., for 3d node positions
		const int   bottom_level_width = 1 << avl_tree->root->height;
		const float max_tree_width = bottom_level_width * (global_node_width + global_node_margin) - global_node_margin; // subtract one node_margin for the far right node
		const float x_start = (max_tree_width / 2.0f) * -1.0f;
		const float y_start = 0.0f;

		// Generate node geometry
		AVLTreeBFSNode bfs_node = AVLTree_BFS(avl_tree);
		while(bfs_node.node) {
			AVLNode* node = bfs_node.node;

			float split = exp2f((float)(bfs_node.level + 1));
			float x_width = (max_tree_width / split);
			// map node indices to odd #'s
			uint64_t split_index = (bfs_node.level_index * 2) + 1;
			// x_pos, y_pos is the center of the node
			float x_pos = x_start + ((float)split_index * x_width);
			float y_pos = y_start - ((float)bfs_node.level * global_y_spacing);

			// Generate all nodes at the origin and set their
			// destinations to their proper positions in the tree.
			// Destinations are based on top left corner (first cube index)
			// of the GameCube.
			float r = 0.0f; //(float)rand() / (float)RAND_MAX;
			float g = 0.0f; //(float)rand() / (float)RAND_MAX;
			float b = 1.0f; //(float)rand() / (float)RAND_MAX;
			node->cube = GenCube(0.0f, 0.0f, 0.0f, node->val, r, g, b);
			// TODO: This would be half node width if I add that as a parameter in engine.cpp
			//       where the nodes are defined and generated.
			node->x_dest = x_pos - 0.5f;
			node->y_dest = y_pos + 0.5f;

			AVLTree_SetVelocity(node);

			bfs_node = AVLTree_BFS(avl_tree);
		}
	}

	avl_tree->current_state = AVLTREE_INITIALIZING;
	avl_tree->previous_state = AVLTREE_PAUSED;
	avl_tree->camera.x = 0.0f;
	avl_tree->camera.y = 5.0f;
	avl_tree->camera.z = -15.0f;

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
