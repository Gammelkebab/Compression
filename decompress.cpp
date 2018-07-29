#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <mpi.h>

#include "decompress.h"

#include "tree.h"

extern struct node **root;
extern const int readIn;

void charToBin(int c, char *output)
{
	for (int i = 7; i >= 0; --i)
	{
		output[7 - i] = (c & (1 << i)) ? '1' : '0';
	}
}

/*
// input compressed txt
// gets converted into bit array
// repeat until all bits are read:
// 		tree traversal from root following left/right childs based on bits
        until leaf is reached, write back character from leaf
*/
int decode(char chrInput[], int inputsize, struct node **root, char chrOutput[])
{
	//bit array
	char *strBin = new char[inputsize * 8]();
	int str[inputsize];

	//char to int conversion returns values [-127,127]
	//getting original int by adding 256
	for (int i = 0; i < inputsize; ++i)
	{
		str[i] = chrInput[i];
		str[i] = (str[i] + 256) % 256;
	}

	//add new bits from previous decoding
	for (int i = 0; i < inputsize; ++i)
	{
		charToBin(str[i], &strBin[i * 8]);
	}

	int j = 0;
	for (int i = 0; i < inputsize * 8; ++i)
	{
		struct node *curr_node = root[0];
		//tree traversal
		//strBin has characters {'0', '1'}, their integer ascii value is {48,49}
		while (curr_node->count != 1 && i < inputsize * 8)
		{
			if (strBin[i] == 48)
			{
				curr_node = curr_node->left;
			}
			else
			{
				curr_node = curr_node->right;
			}
			++i;
		}
		//leaf found
		if (curr_node->count == 1)
		{
			chrOutput[j] = curr_node->values[0];
			//64 or '@' is stopword! do not count this leaf as character from original text
			//end of this compressed block is reached
			//break before incrementing j!
			if (chrOutput[j] == 64)
				break;
			j++;
			--i;
		}
	}

	//write back
	delete[] strBin;

	return j;
}

int decodeText(char filename_in[], char filename_out[], struct node **root, int proc_amt, int proc_num)
{
	MPI_File input_file;
	MPI_File_open(MPI_COMM_WORLD, filename_in, MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);

	MPI_File output_file;
	MPI_File_open(MPI_COMM_WORLD, filename_out, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &output_file);

	int worker_amt = proc_amt;

	long long block_amt;
	MPI_File_read_at(input_file, 0, &block_amt, 1, MPI_LONG_LONG, MPI_STATUS_IGNORE);

	int block_size_max = readIn;
	int block_sizes[block_amt];

	int read_offset = 8 + 4 * block_amt;

	for (int block = 0; block < block_amt; block++)
	{
		int worker = block % proc_amt;

		/* Read file */
		int read_offset_header = 8 + 4 * block;

		MPI_File_read_at(input_file, read_offset_header, &block_sizes[block], 1, MPI_INT, MPI_STATUS_IGNORE);

		if (worker == proc_num)
		{
			char read_buffer[block_sizes[block]];
			MPI_File_read_at(input_file, read_offset, read_buffer, block_sizes[block], MPI_CHAR, MPI_STATUS_IGNORE);

			char write_buffer[block_size_max];
			int bytes_decoded = decode(read_buffer, block_sizes[block], root, write_buffer);

			int write_offset = block_size_max * block;
			MPI_File_write_at(output_file, write_offset, write_buffer, bytes_decoded, MPI_CHAR, MPI_STATUS_IGNORE);
		}

		read_offset += block_sizes[block];
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_File_close(&input_file);
	MPI_File_close(&output_file);

	return 1;
}
