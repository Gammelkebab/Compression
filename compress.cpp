#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "compress.h"
#include "tree.h"

extern int readIn;
extern int symbols;

void countSymbols(char data[], int size, int* letters) {
#pragma omp parallel for schedule(static, 256) // CHANGED
	for (int i=0; i<size; ++i) {
		letters[(int)data[i]]++;
	}
}

/**
//read a simple .txt file and count occurence of symbols
**/
int readData(const char* filename, int* letters) {
	FILE *fp;
	int last_pos = 0;
	int curr_pos = 0;
	char str[readIn];

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Could not open file %s\n",filename);
		return 0;
	}
	while (fgets(str, readIn, fp) != NULL) {
		curr_pos = ftell(fp);
		int inputsize = curr_pos - last_pos;
		last_pos = curr_pos;
		countSymbols(str, inputsize, letters);
	}

	fclose(fp);
	return 1;
}

//print frequency of each occuring symbol
void printFreq(int* letters) {
	for(int i=0;i<symbols;++i) {
		if(letters[i] > 0) {
			printf("%c: %d\n",i,letters[i]);
		}
	}
}

/*
// returns the ~size of a file
// only for binary files accurate?!
*/
long int getFileSize(FILE* fpIn) {
	fseek (fpIn, 0, SEEK_END);
	long int size = ftell(fpIn);
	fseek (fpIn, 0, SEEK_SET);

	return size;
}

/*
//first store the number of blocks (long int)
//then the size of each block (int)
*/
void createHeader(FILE* fpOut, int* blockSizes, long int count) {
	fwrite(&count, sizeof(long int), 1, fpOut);
	for(long int i=0; i<count; ++i) {
		fwrite(&blockSizes[i], sizeof(int), 1, fpOut);
	}
}


/**
// receives two strings and their sizes, encodes each symbol in a bit-sequence and stores every 8 bit as a single char
// this single char is then written back in the final output file
// returns the index of the last symbol that could not be encoded since full 8 bit where not reached
// these last symbols are encoded the next time this function is called -> reason for input of 2 strings and not just one
**/
int writeAsBinary(struct key_value* binEncoding, char str[], int inputsize, FILE* fpOut) {
	char out[inputsize];
	int num_bytes = 0;
	int bin_exp = 7;
	int full_byte = 0;

	for(int i=0; i<inputsize; ++i) {
		char* bin = key(binEncoding, str[i]);
		//first value of bin is size of bin-1
		for(int j=1; j<bin[0]+1; ++j) {
			full_byte += bin[j]*pow(2,bin_exp);
			bin_exp--;
			//8 bits processed
			if(bin_exp == -1) {
				out[num_bytes] = full_byte;
				full_byte = 0;
				bin_exp = 7;
				num_bytes++;
			}
		}
	}

	if(bin_exp != 7) {
		out[num_bytes] = full_byte;
		num_bytes++;
	}

	fwrite(&out[0], sizeof(char), num_bytes, fpOut);
	return num_bytes;
}

/*
// encodes 'filename' with the given encoding 'binEncoding'
// the file is split up in blocks of size 'readIn' (global value)
// each block is compressed seperately
*/
int encodeTextFile(char filename[], char output[], struct key_value* binEncoding) {
	FILE *fpIn, *fpOut;
	char str[readIn+1];

	fpIn = fopen(filename, "r");
	fpOut = fopen(output, "w+b");

	//check files
	if (fpIn == NULL) {
	    printf("Could not open file %s\n",filename);
	    return 0;
	} else if(fpOut == NULL) {
		printf("Could not create file %s\n",output);
	    return 0;
	}

	long int size = getFileSize(fpIn);
	long int partitions = size / readIn +1; // round up
	int* blockSizes = new int[partitions]();
	long int i;

	//we do not know how big each block is after compressing
	//but we know how many blocks we have (long int)
	//and the size we need to store the size of each block (int)
	//since header should be at beginning of file, skip it
	//header will be written later
	fseek(fpOut, sizeof(long int)+partitions*sizeof(int), SEEK_SET);
	bool tmp = true;
//	#pragma omp parallel for schedule(static, 256) // CHANGED
	for(i=0; i<partitions; ++i) {
		if(tmp){
			//read+1 because '\0' is added automatically
			int len = fread(str, sizeof(char), readIn+1, fpIn);
			//add stopword '@' to end of block
			str[len] = '@';
			blockSizes[i] = writeAsBinary(binEncoding, str, len, fpOut);

			//eof, break loop
			if(blockSizes[i] == 0) {
				tmp = false;
			}
		}
	}

	//set pointer back to beginning of file
	fseek(fpOut, 0, SEEK_SET);
	createHeader(fpOut, blockSizes, i);

	fclose(fpIn);
	fclose(fpOut);
	return 1;
}
