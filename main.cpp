#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include "tree.h"
#include "compress.h"
#include "decompress.h"
using namespace std;
int symbols = 128;
int readIn = 65536;

struct node** root;

int main(int argc, char** argv) {
    //time measurement
    double elapsed = 0;
    struct timeval begin, end;

	setbuf(stdout, NULL);
	char filename[2][100];
	strcpy(filename[0], "lorem_ipsum.txt");
	strcpy(filename[1], "uncompressed.txt");
	for(int i=1; i<argc; ++i) {
		if(i>2) break;
		strcpy(filename[i-1],argv[i]);
	}

    //create name for compressed output file
	int len = strlen(filename[0]);
	char fileEncoded[100];
	strncpy(fileEncoded, filename[0], len-3);
	fileEncoded[len-3] = '\0';
	strcat(fileEncoded, "bin");

	int* letters = new int[symbols]();

	//read data and create huffmann tree
	if(!readData(filename[0], letters)) { // WICHTIG1
		return 0;
	}
//	printFreq();
	root = createTree(letters);
	struct key_value* binEncoding = new struct key_value[root[0]->count]();
	encoding(binEncoding, root[0]);

//	printTree(root, 1);
//	printEncoding(binEncoding, root[0]->count);

    gettimeofday(&begin, NULL);

	//start actual encoding
	printf("start compressing ...");
	encodeTextFile(filename[0], fileEncoded,  binEncoding); // WICHTIG2
	printf("done!\n");

    /*****************************
    ******************************
    ****PARALLEL BARRIER HERE
    ****Compressing of text file has to be completed,
    ****before decompressing starts
    ****do not compress and decompress at the same time
    ******************************
    *****************************/

	//decode
	printf("start decompressing ...");
	decodeText(fileEncoded, filename[1], root); // WICHTIG3
	printf("done!\n");

    gettimeofday(&end, NULL);
    elapsed += (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
    printf("Runtime: %.5fs\n",elapsed);

	delete[] letters;
	delete[] binEncoding;
}
