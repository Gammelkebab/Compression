#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "decompress.h"
#include <cstring>

extern struct node** root;
extern int readIn;

void charToBin(int c, char* output) {
	for (int i = 7; i >= 0; --i) {
		output[7-i] = (c & (1 << i)) ? '1' : '0' ;
	}
}

/*
// input compressed txt
// gets converted into bit array
// repeat until all bits are read:
// 		tree traversal from root following left/right childs based on bits
        until leaf is reached, write back character from leaf
*/
void decode(char chrInput[], int inputsize, struct node** root, FILE* fp) {
	//bit array
	char* strBin = new char[inputsize*8]();
	char output[readIn*2];
	int str[inputsize];

	//char to int conversion returns values [-127,127]
	//getting original int by adding 256
	for(int i=0; i<inputsize; ++i) {
		str[i] = chrInput[i];
		str[i] = (str[i]+256)%256;
	}

	//add new bits from previous decoding
	for(int i=0; i<inputsize; ++i) {
		charToBin(str[i], &strBin[i*8]);
	}

	int j = 0;
	for(int i=0; i<inputsize*8; ++i) {
		struct node* curr_node = root[0];
		//tree traversal
		//strBin has characters {'0', '1'}, their integer ascii value is {48,49}
		while(curr_node->count != 1 && i<inputsize*8) {
			if(strBin[i] == 48) {
				curr_node = curr_node->left;
			} else {
				curr_node = curr_node->right;
			}
			++i;
		}
		//leaf found
		if(curr_node->count == 1) {
			output[j] = curr_node->values[0];
			//64 or '@' is stopword! do not count this leaf as character from original text
			//end of this compressed block is reached
			//break before incrementing j!
			if(output[j] == 64) break;
			j++;
			--i;
		}
	}

	//write back
	fwrite(output, sizeof(char), j, fp);
	delete[] strBin;

}

/*
//This function decodes 'filename' with the huffmann tree 'root'
//first the header is read
//second each block of data is decoded
*/
int decodeText(char filename[], char output[], struct node** root) {
	FILE* fpIn;
	FILE* fpOut;
	char str[readIn];
	memset(str, 0, readIn*sizeof(char));

	fpIn = fopen(filename, "rb");
	fpOut = fopen(output, "w+");
	if (fpIn == NULL) {
	    printf("Could not open file %s\n",filename);
	    return 0;
	} else if(fpOut == NULL) {
		printf("Could not create file %s\n",output);
	    return 0;
	}


	//read header
	//first 8 byte indicate number of blocks
	//then 4 byte for each block, representing the size of this block
	long int num_blocks = 0;
	fread(&num_blocks, sizeof(long int), 1, fpIn);
	int* blockSizes = new int[num_blocks]();
	fread(blockSizes, sizeof(int), num_blocks, fpIn);

	//decode each block and write result immediately back
	for(long int i=0; i<num_blocks; ++i) {
		fread(&str[0], sizeof(char), blockSizes[i], fpIn);
		decode(str, blockSizes[i], root, fpOut);
	}


	delete[] blockSizes;
	fclose(fpIn);
	fclose(fpOut);
	return 1;
}
