#ifndef COMPRESS_H
#define COMPRESS_H 1

void countSymbols(char data[], int size, int* letters);
/**
//read a simple .txt file and count occurence of symbols
**/
int readData(const char* filename, int* letters);

//print frequency of each occuring symbol
void printFreq(int* letters);

/*
// returns ~the size of a file
// only for binary files accurate?!
*/
long int getFileSize(FILE* fpIn);

/*
//first store the number of blocks (long int)
//then the size of each block (int)
*/
void createHeader(FILE* fpIn, int* blockSizes, long int count);

/**
// receives two strings and their sizes, encodes each symbol in a bit-format and stores every 8 bit as a single char
// this single char is then written back in the final output file
// returns the index of the last symbols that could not be encoded since 8 bit where not reached
// these last symbols are encoded the next time this function is called -> reason for input of 2 strings and not just one
**/
int writeAsBinary(struct key_value* binEncoding, char str[], int inputsize, FILE* fpOut);

/*
// encodes 'filename' with the given encoding 'binEncoding'
// the file is split up in blocks of size readIn (global value)
// each block is compressed seperately
*/
int encodeTextFile(char filename[], char output[], struct key_value* binEncoding, int proc_amt, int proc_num);

#endif
