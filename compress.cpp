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

	for (int i = 0; i < inputsize; ++i)
	{
		char *bin = key(binEncoding, str_in[i]);
		//first value of bin is size of bin-1
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

	int worker_amt = proc_amt - 1;

	int block_size_max = readIn;
	int block_size_min = size % block_size_max;
	long long block_amt = size / block_size_max + (size % block_size_max == 0 ? 0 : 1); // ! keep this long long, otherwise the header write fails

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

		// Start receiving
		MPI_Request recv_requests[block_amt];
		
		
	    double elapsed = 0;
	    double tmp_el = 0;
	    struct timeval begin, end;
	    
	    
		for (int block = 0; block < block_amt; block++)
		{
			int worker = block % worker_amt + 1;
			MPI_Irecv(&write_buffers[block], block_size_max, MPI_CHAR, worker, block, MPI_COMM_WORLD, &recv_requests[block]);
		}
				
		MPI_File output_file;
		MPI_File_open(MPI_COMM_SELF, filename_out, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &output_file);
        

		MPI_File_write_at(output_file, 0, &block_amt, 1, MPI_LONG, MPI_STATUS_IGNORE);
		long long block_offset = 8 + 4 * block_amt;

		
	    gettimeofday(&begin, NULL);
	    
		for (int block = 0; block < block_amt; block++)
		{
			MPI_Status recv_status;
			MPI_Wait(&recv_requests[block], &recv_status);
			MPI_Get_count(&recv_status, MPI_CHAR, &write_buffers_size[block]);

			int block_header_offset = 8 + 4 * block;
			MPI_File_write_at(output_file, block_header_offset, &write_buffers_size[block], 1, MPI_INTEGER, MPI_STATUS_IGNORE);

			MPI_File_write_at(output_file, block_offset, write_buffers[block], write_buffers_size[block], MPI_CHAR, MPI_STATUS_IGNORE);
			block_offset += write_buffers_size[block];
		}

        
	    gettimeofday(&end, NULL);
    	elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec) / 1000000.0);
		printf("Runtime of File_open + File_write_at: %.5fs\n", elapsed);
		
		MPI_File_close(&output_file);
	}
	else // Not Processor 0
	{
		for (int block = proc_num - 1; block < block_amt; block += worker_amt)
		{
			// Read file
			int offset = block_size_max * block;
			int block_size = block == block_amt - 1 ? block_size_min : block_size_max;

			char read_buffer[block_size];
			MPI_File_read_at(input_file, offset, read_buffer, block_size, MPI_CHAR, MPI_STATUS_IGNORE);

			long long bytes_encoded;
			char write_buffer[block_size]; // Never more chars after compression then before
			bytes_encoded = writeAsBinary(bin_encoding, read_buffer, block_size, write_buffer);

			MPI_Request send_request;
			MPI_Isend(write_buffer, bytes_encoded, MPI_CHAR, 0, block, MPI_COMM_WORLD, &send_request); // Send encoded data
		}
	}

	MPI_File_close(&input_file);

	return 1;
}
