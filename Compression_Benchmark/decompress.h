#ifndef DECOMPRESS_H
#define DECOMPRESS_H 1


void charToBin(int c, char* output);

/*
// input compressed txt
// gets converted into bit array
// repeat until all bits are read:
// 		tree traversal from roots with bits until leaf is reached, write back of character from leaf
*/
int decode(char chrInput[], int inputsize, struct node** root, char chrOutput[]);
/*
//This function decodes 'filename' with the huffmann tree 'root'
//first the header is read
//second each block of data is decoded
*/
int decodeText(char filename[], char output[], struct node** root);

#endif
