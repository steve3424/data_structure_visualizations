#include "engine.h"
#include "windows.h"

#define AVL_TREE_SIZE 15
#define THRESHOLD 0.001f

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

// TODO: Do I need to track size?
typedef struct {
	// data
	int size;
	AVLNode* root;

	// opengl
	GameCamera camera;
	unsigned int vbo;
	unsigned int shader;
	GameBackground background;

	// state machine stuff
	AVLTreeState current_state;
	AVLTreeState previous_state;
} AVLTree;

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

static AVLTree* AVLTree_Init() {
	AVLTree* avl_tree = (AVLTree*)calloc(1, sizeof(AVLTree));
	if(!avl_tree) {
		fprintf(stderr, "Couldn't malloc for ISort\n");
		return NULL;
	}

	// Initialize entire tree to get final positions in tree
	for(int i = 0; i < AVL_TREE_SIZE; ++i) {
		int val = rand() % MAX_DIGITS;
		AVLTree_Insert(avl_tree, val);
	}

	// Initialize geometry
	
	// Set up spacing, width, ...
	int bottom_level_width = 1;
	for(int i = 0; i < avl_tree->root->height; ++i) {
		bottom_level_width *= 2;
	}
	const float node_width = 1.0f; // this is fixed based on the model sent to the GPU
	const float node_margin = 0.7f; // space between the nodes at the bottom level
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

			uint64_t split_index = (current_index * 2) + 1; // map node indices to odd #'s
			float x_pos = x_start + ((float)split_index * x_width);
			current_node->cube = GenCube(x_pos, y_pos, 0.0f, current_node->val, 0.0f, 0.0f, 1.0f);

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

	// Initialize state
	avl_tree->camera.x = 0.0f;
	avl_tree->camera.z = -12.0f;

	// Initialize opengl stuff
	avl_tree->shader = LoadShaderProgram("..\\zshaders\\game_cube.vert", "..\\zshaders\\game_cube.frag");

	GLCall(glGenBuffers(1, &avl_tree->vbo));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, avl_tree->vbo));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, pos))));
	GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, rgb))));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glEnableVertexAttribArray(1));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	return avl_tree;
}

static void AVLTree_Draw(AVLTree* avl_tree, float window_width, float window_height) {
	assert(avl_tree);
	assert(0.0f < window_width);
	assert(0.0f < window_height);

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, avl_tree->vbo));
	GLCall(glUseProgram(avl_tree->shader));

	unsigned int buffer_size = avl_tree->size * sizeof(GameCube);
	GLCall(glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW));

	// BFS
	int push = 0;
	int pop = 0;
	int size = 0;
	uint64_t node_indices[MAX_DIGITS] = {0};
	AVLNode* nodes[MAX_DIGITS] = {NULL};
	nodes[push] = avl_tree->root;
	++push;
	++size;
	unsigned int buffer_write_index = 0;
	while(size > 0) {
		int nodes_at_this_level = size;
		for(int i = 0; i < nodes_at_this_level; ++i) {
			AVLNode* current_node = nodes[pop];
			uint64_t current_index = node_indices[pop];
			pop = (pop + 1) % MAX_DIGITS;
			--size;

			GLCall(glBufferSubData(GL_ARRAY_BUFFER, buffer_write_index, sizeof(GameCube), 
						           &current_node->cube));
			buffer_write_index += sizeof(GameCube);

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
	}

	GLCall(glLineWidth(4.0f));
	GLCall(glEnable(GL_DEPTH_TEST));

	int model_location = glGetUniformLocation(avl_tree->shader, "model");
	int view_location = glGetUniformLocation(avl_tree->shader, "view");
	int projection_location = glGetUniformLocation(avl_tree->shader, "projection");

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(avl_tree->camera.x, 
					                      avl_tree->camera.y, 
					                      avl_tree->camera.z));
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), 
			                                (float)window_width / (float)window_height, 
											0.1f, 100.0f);

	glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

	unsigned int vertices_per_cube = sizeof(GameCube) / sizeof(Vertex);
	GLCall(glDrawArrays(GL_LINES, 0, avl_tree->size * VERTICES_PER_CUBE));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
