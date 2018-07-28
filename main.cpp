#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include <mpi.h>

#include "tree.h"
#include "compress.h"
#include "decompress.h"
using namespace std;

int symbols = 128;
int readIn = 8192;

struct node **root;

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	//time measurement
	double elapsed = 0;
	double tmp_el = 0;
	struct timeval begin, end, tmp1, tmp2;

	// Setup MPI

	int proc_amt, proc_num;

	MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_amt);

	if (proc_num == 0)
	{
		printf("proc_amt: %d\n", proc_amt);
	}

	char processor_name[100];
	int processor_name_length;
	MPI_Get_processor_name(processor_name, &processor_name_length);
	printf("%d \t=> %s\n", proc_num, processor_name);

	// Setup files

	setbuf(stdout, NULL);
	char filename[2][100];
	strcpy(filename[0], "lorem_ipsum.txt");
	strcpy(filename[1], "uncompressed.txt");
	for (int i = 1; i < argc; ++i)
	{
		if (i > 2)
			break;
		strcpy(filename[i - 1], argv[i]);
	}

	//create name for compressed output file
	int len = strlen(filename[0]);
	char fileEncoded[100];
	strncpy(fileEncoded, filename[0], len - 3);
	fileEncoded[len - 3] = '\0';
	strcat(fileEncoded, "bin");

	int *letters = new int[symbols]();

	//read data and create huffmann tree
	if (!readData(filename[0], letters))
	{
		return 0;
	}

	root = createTree(letters);
	struct key_value *binEncoding = new struct key_value[root[0]->count]();
	encoding(binEncoding, root[0]);

	gettimeofday(&begin, NULL);

	// Encoding
	if (proc_num == 0)
		printf("start compressing ...\n");
	gettimeofday(&tmp1, NULL);
	encodeTextFile(filename[0], fileEncoded, binEncoding, proc_amt, proc_num);
	if (proc_num == 0)
		printf("done!\n");
	gettimeofday(&tmp2, NULL);
	tmp_el = (tmp2.tv_sec - tmp1.tv_sec) + ((tmp2.tv_usec - tmp1.tv_usec) / 1000000.0);
	if (proc_num == 0)
		printf("Compression: %.5fs\n", tmp_el);

	// Compressing of text file has to be completed,
	// before decompressing starts do not compress and decompress at the same time
	MPI_Barrier(MPI_COMM_WORLD);

	// Decoding
	gettimeofday(&tmp1, NULL);
	if (proc_num == 0)
		printf("start decompressing ...\n");
	if (proc_num == 0)
	{
		decodeText(fileEncoded, filename[1], root);
	}
	if (proc_num == 0)
		printf("done!\n");
	gettimeofday(&tmp2, NULL);
	tmp_el = (tmp2.tv_sec - tmp1.tv_sec) + ((tmp2.tv_usec - tmp1.tv_usec) / 1000000.0);
	if (proc_num == 0)
		printf("Decompression: %.5fs\n", tmp_el);

	gettimeofday(&end, NULL);
	elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec) / 1000000.0);
	if (proc_num == 0)
		printf("Runtime: %.5fs, Speedup: %.5fx\n", elapsed, (0.23 / elapsed));

	delete[] letters;
	delete[] binEncoding;

	MPI_Finalize();
}
