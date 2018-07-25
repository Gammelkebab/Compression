#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include "tree.h"

extern int symbols;
extern struct node** root;

char* key(struct key_value* binEncoding, int key) {
	for(int i=0; i<root[0]->count; ++i) {
		if(binEncoding[i].key == key) {
			return binEncoding[i].value;
		}
	}
	return NULL;
}

void addEncoding(struct key_value* binEncoding, struct node* nodes, int* step, int binValues[], int depth) {
	if (nodes->count == 1) {
		char* binRepresentation = new char[depth+1]();
		binRepresentation[0] = depth;
		for(int i=0; i<depth; ++i) {
			binRepresentation[i+1] = (char) binValues[i];
		}
		//binRepresentation[depth] = '\0';
		binEncoding[step[0]].value = binRepresentation;
		binEncoding[step[0]].key = nodes->values[0];
		step[0]++;
		return;
	}
	binValues[depth] = 0;
	addEncoding(binEncoding, nodes->left, step, binValues, depth+1);
	binValues[depth] = 1;
	addEncoding(binEncoding, nodes->right, step, binValues, depth+1);
}

void encoding(struct key_value* binEncoding, struct node* nodes) {
	int* step = new int[1]();
	step[0] = 0;
	int binValues[(int)log(nodes->count)+1];
	int depth = 0;

	// ascii 48 = 0
	// ascii 49 = 1
	binValues[depth] = 0;
	addEncoding(binEncoding, nodes->left, step, binValues, depth+1);
	depth = 0;
	binValues[depth] = 1;
	addEncoding(binEncoding, nodes->right, step, binValues, depth+1);
}

//prints character and their binary encoding
void printEncoding(struct key_value* Encoding, int size) {
	for(int i=0; i<size; ++i) {
		if(Encoding[i].key != 10)
			printf("%c:",Encoding[i].key);
		else
			printf("\\n:");
		for(int j=1; j<Encoding[i].value[0]+1; ++j) {
			printf("%d", Encoding[i].value[j]);
		}
		printf("\n");
	}

}

//prints the encoding tree
void printTree(struct node** level, int size, int curr_level) {
	int notLeaves = 0;
	printf("level: %d\t",curr_level);
	for(int i=0; i<size; ++i) {
		printf("(");
		for(int j=0; j<level[i]->count; ++j) {
			printf("%c,",level[i]->values[j]);
		}
		if(level[i]->count > 1) notLeaves++;
		printf("%d) ",level[i]->freq);
	}
	printf("\n");

	if (notLeaves == 0) {
		delete[] level;
		return;
	}
	int count = 0;
	struct node** next_level = new struct node*[notLeaves*2]();
	for(int i=0; i<size; ++i) {
		if(level[i]->count > 1) {
			next_level[count] = level[i]->left;
			next_level[count+1] = level[i]->right;
			count += 2;
		}
	}
	printTree(next_level, notLeaves*2, curr_level+1);
	if(curr_level > 1) {
		delete[] level;
	}
}

//returns index of lowest value in 'data'
//value must be > 0 !
int retrieveIndexOfLowestValue(int* data) {
	int min = INT_MAX;
	int index = 0;
	for(int i=0;i<symbols; ++i) {
		if(data[i] > 0 && data[i] < min) {
			min = data[i];
			index = i;
		}
	}
	return index;
}

//returns Index of lowest value in 'nodes'
//value must be > 0!
//this function is called 2 times to get the two lowest values
//to prevent getting the same value twice, 
//prevIndex denotes the index of the lowest value already found
int retrieveIndexOfLowestValue(struct node** nodes, int size, int prevIndex) {
	int min = INT_MAX;
	int index = 0;
	for(int i=0;i<size; ++i) {
		if(nodes[i] != NULL && nodes[i]->freq > 0 && nodes[i]->freq < min) {
			if(prevIndex != i) {
				min = nodes[i]->freq;
				index = i;
			}
		}
	}
	return index;
}

//merges 2 nodes into one parent node
void mergeChilds(struct node* parent, struct node** second_level, int size) {
	int low1 = retrieveIndexOfLowestValue(second_level, size);
	int low2 = retrieveIndexOfLowestValue(second_level, size, low1);

    //add frequency, number of different symbols and the symbols itself
	parent->freq = second_level[low1]->freq + second_level[low2]->freq;
	parent->count = second_level[low1]->count + second_level[low2]->count;
	parent->values = new int[parent->count]();
	for(int i=0; i<second_level[low1]->count; ++i) {
		parent->values[i] = second_level[low1]->values[i];
	}
	for(int i=0; i<second_level[low2]->count; ++i) {
		parent->values[i+second_level[low1]->count] = second_level[low2]->values[i];
	}

    //add the 2 nodes as childs to parent node
    //remove them from second_level, because they are already merged
	parent->left = second_level[low1];
	parent->right = second_level[low2];
	second_level[low1] = NULL;
	second_level[low2] = NULL;
}

//creates a huffmann-tree
//data[i] denotes how often ascii value i appeared in the text
struct node** createTree(int* data) {
	int size = 0;
    //special delimiter
	data[64] = 1;
	for(int i=0; i<symbols; ++i) {
		if(data[i] > 0) size++;
	}
	struct node* leaves = new struct node[size]();
	struct node** second_level = new struct node*[size]();

	//create leaves
	//filter out ascii values without occourence
	for(int i=0; i<size; ++i) {
		int j = retrieveIndexOfLowestValue(&data[0]);
		leaves[i].freq = data[j];
		leaves[i].values = new int[1]();
		leaves[i].values[0] = j;
		leaves[i].count = 1;
		second_level[i] = &leaves[i];
		data[j] = 0;
	}
	struct node** top_level;

    //until only one parent node is left, merge nodes together
	while(size > 1) {
		top_level = new struct node*[size/2+size%2]();
		for(int i=0; i<size/2; ++i) {
			struct node* parent = new struct node[1]();
			mergeChilds(parent, second_level, size);
			top_level[i] = parent;
		}
        //one node is left and can not be merged in this iteration
		if(size%2) {
			int remaining = retrieveIndexOfLowestValue(second_level, size);
			top_level[size/2] = second_level[remaining];
		}
		delete[] second_level;
		second_level = top_level;
		size = size/2+size%2;
	}
	return top_level;
}
