#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <sys/time.h>

#include <mpi.h>

#include "compress.h"
#include "tree.h"

extern int readIn;
extern int symbols;
double tmp_el = 0;

void countSymbols(char data[], int size, int *letters)
{
	for (int i = 0; i < size; ++i)
	{
		letters[(int)data[i]]++;
	}
}

/**
//read a simple .txt file and count occurence of symbols
**/
int readData(const char *filename, int *letters)
{ //WICHTIG1
	FILE *fp;
	int last_pos = 0;
	int curr_pos = 0;
	char str[readIn];

	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		printf("Could not open file %s\n", filename);
		return 0;
	}
	while (fgets(str, readIn, fp) != NULL)
	{
		curr_pos = ftell(fp);
		int inputsize = curr_pos - last_pos;
		last_pos = curr_pos;
		countSymbols(str, inputsize, letters);
	}

	fclose(fp);
	return 1;
}

//print frequency of each occuring symbol
void printFreq(int *letters)
{
	for (int i = 0; i < symbols; ++i)
	{
		if (letters[i] > 0)
		{
			printf("%c: %d\n", i, letters[i]);
		}
	}
}

/*
// returns the ~size of a file
// only for binary files accurate?!
*/
long int getFileSize(FILE *fpIn)
{
	fseek(fpIn, 0, SEEK_END);
	long int size = ftell(fpIn);
	fseek(fpIn, 0, SEEK_SET);

	return size;
}

/*
//first store the number of blocks (long int)
//then the size of each block (int)
*/
void createHeader(FILE *fpOut, int *blockSizes, long int count)
{
	fwrite(&count, sizeof(long int), 1, fpOut);
	for (long int i = 0; i < count; ++i)
	{
		fwrite(&blockSizes[i], sizeof(int), 1, fpOut);
	}
}

/*
 * receives two strings and their sizes, encodes each symbol in a bit-sequence and stores every 8 bit as a single char
 * this single char is then written back in the final output file
 * returns the index of the last symbol that could not be encoded since full 8 bit where not reached
 * these last symbols are encoded the next time this function is called -> reason for input of 2 strings and not just one
 */
int writeAsBinary(struct key_value *binEncoding, char str_in[], int inputsize, char str_out[])
{
	int num_bytes = 0;
	int bin_exp = 7;
	int full_byte = 0;

	//printf("inputsize: %d\n",inputsize);
	for (int i = 0; i < inputsize; ++i)
	{
		char *bin = key(binEncoding, str_in[i]);
		//first value of bin is size of bin-1
		//printf("bin[0]+1: %d\n",(bin[0]+1));
		for (int j = 1; j < bin[0] + 1; ++j)
		{
			full_byte += bin[j] * pow(2, bin_exp);
			bin_exp--;
			//8 bits processed
			if (bin_exp == -1)
			{
				str_out[num_bytes] = full_byte;
				full_byte = 0;
				bin_exp = 7;
				num_bytes++;
			}
		}
	}

	if (bin_exp != 7)
	{
		str_out[num_bytes] = full_byte;
		num_bytes++;
	}

	return num_bytes;
}

/*
 * encodes 'filename' with the given encoding 'binEncoding'
 * the file is split up in blocks of size 'readIn' (global value)
 * each block is compressed seperately
 */
int encodeTextFile(char filename_in[], char filename_out[], struct key_value *bin_encoding, int proc_amt, int proc_num)
{
	MPI_File input_file;
	MPI_File_open(MPI_COMM_WORLD, filename_in, MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);

	MPI_Offset size;
	MPI_File_get_size(input_file, &size);

	int block_size_max = readIn;
	int block_size_min = size % block_size_max;
	int block_amt = size / block_size_max + (size % block_size_max == 0 ? 0 : 1);

	printf("size: %d, bsmax: %d, bsmin: %d, bam: %d\n", size, block_size_max, block_size_min, block_amt);

	/*
	 * we do not know how big each block is after compressing
	 * but we know how many blocks we have (long int)
	 * and the size we need to store the size of each block (int)
	 * since header should be at beginning of file, skip it
	 * header will be written later
	 */

	if (proc_num == 0) // This is the master thread
	{
		int write_buffers_size[block_amt];
		char write_buffers[block_amt][block_size_max]; // Never more chars after compression then before

		for (int block = 0; block < block_amt; block++)
		{
			int worker = block % proc_amt;

			if (worker == 0)
			{
				// Read file
				int offset = block_size_max * block;
				int block_size = block == block_amt - 1 ? block_size_min : block_size_max;
				char read_buffer[block_size];
				MPI_File_read_at(input_file, offset, read_buffer, block_size, MPI_CHAR, MPI_STATUS_IGNORE);

				long long write_buffer_size;
				char write_buffer[block_size]; // Never more chars after compression then before
				write_buffers_size[block] = writeAsBinary(bin_encoding, read_buffer, block_size, write_buffers[block]);
			}
			else
			{
				MPI_Recv(&write_buffers_size[block], 1, MPI_LONG_LONG, worker, block, MPI_COMM_WORLD, MPI_STATUS_IGNORE);				 // Receive amount of encoded data
				MPI_Recv(write_buffers[block], write_buffers_size[block], MPI_CHAR, worker, block, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive encoded data
			}
		}

		for (int i = 0; i < block_amt; i++)
		{
			printf("Buffer %d: %d\n", i, write_buffers_size[i]);
		}

		MPI_File output_file;
		MPI_File_open(MPI_COMM_SELF, filename_out, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &output_file);

		long long offset = 0;
		MPI_File_write_at(output_file, offset, &block_amt, 1, MPI_LONG, MPI_STATUS_IGNORE);
		offset += 8;
		MPI_File_write_at(output_file, offset, write_buffers_size, block_amt, MPI_INTEGER, MPI_STATUS_IGNORE);
		offset += 4 * block_amt;

		for (int block = 0; block < block_amt; block++)
		{
			MPI_File_write_at(output_file, offset, write_buffers[block], write_buffers_size[block], MPI_CHAR, MPI_STATUS_IGNORE);
			offset += write_buffers_size[block];
		}
	}
	else // Not Processor 0
	{
		for (int block = proc_num; block < block_amt; block += proc_amt)
		{
			// Read file
			int offset = block_size_max * block;
			int block_size = block == block_amt - 1 ? block_size_min : block_size_max;
			printf("bsize: %d\n", block_size);

			char read_buffer[block_size];
			MPI_File_read_at(input_file, offset, read_buffer, block_size, MPI_CHAR, MPI_STATUS_IGNORE);

			long long write_buffer_size;
			char write_buffer[block_size]; // Never more chars after compression then before
			write_buffer_size = writeAsBinary(bin_encoding, read_buffer, block_size, write_buffer);

			MPI_Send(&write_buffer_size, 1, MPI_LONG_LONG, 0, block, MPI_COMM_WORLD);	  // Send size of encoded data
			MPI_Send(write_buffer, write_buffer_size, MPI_CHAR, 0, block, MPI_COMM_WORLD); // Send encoded data
		}
	}

	MPI_File_close(&input_file);

	return 1;
}
