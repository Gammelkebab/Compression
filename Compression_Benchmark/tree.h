#ifndef TREE_H
#define TREE_H 1

struct key_value {
	int key;
	//first value is number of bits ( == size of array-1)
	//e.g. for 5 bits (01100), array size is 5+1: [5,0,1,1,0,0]
	char* value;
};

struct node {
	int freq;
	int count;
	int* values;
	struct node* left;
	struct node* right;
};

char* key(struct key_value* binEncoding, int key);
void encoding(struct key_value* binEncoding, struct node* nodes);
void printEncoding(struct key_value* Encoding, int size);

void printTree(struct node** level, int size, int curr_level = 1);
int retrieveIndexOfLowestValue(int* data);
int retrieveIndexOfLowestValue(struct node** nodes, int size, int prevIndex = -1);
void mergeChilds(struct node* parent, struct node** second_level, int size);
struct node** createTree(int* data);

#endif
