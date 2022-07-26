/*************************************************************************
* Title		: HPC Game Of Life
* Author	: Dimitri Dening
* Created	: 25.04.2021
* License	: MIT License
*
*		Copyright (C) 2021 Dimitri Dening
*
*		Permission is hereby granted, free of charge, to any person obtaining a copy
*		of this software and associated documentation files (the "Software"), to deal
*		in the Software without restriction, including without limitation the rights
*		to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*		copies of the Software, and to permit persons to whom the Software is
*		furnished to do so, subject to the following conditions:
*
*		The above copyright notice and this permission notice shall be included in all
*		copies or substantial portions of the Software.
*
*		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*		IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*		FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*		LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*		OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*		SOFTWARE.
*
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* User defined headers */
#include "export.h"
#include "mem_optimized.h"
#include "shared_mem.h"
#include "distr_mem.h"
#include "utils.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

int main(int argc, char** argv) {
	if (argc != 9) error("Arguments: <height> <width> <frames> <input.lif> <output1.lif> <output2.lif> <mode> <new_folder_name>");
	
	int process_count;
	int rank;
	int field_size[1];
	int mode;			// 0: both, 1: shared, 2: distributed

	double start_time;
	double end_time;
	double duration;
	double data[1];

	unsigned int width;
	unsigned int height;
	unsigned int frames;

	char* input_field_filename;
	char* output_field_filename1;
	char* output_field_filename2;
	char* export_filename1;
	char* export_filename2;
	char* folder_name;

	size_t array_size;

	width = atoi(argv[1]);
	height = atoi(argv[2]);
	frames = atoi(argv[3]);
	input_field_filename = argv[4];
	output_field_filename1 = argv[5];
	output_field_filename2 = argv[6];
	//export_filename1 = argv[7];
	// export_filename2 = argv[8];
	mode = atoi(argv[7]);
	folder_name = argv[8];

	if (height <= 0) error("Height must be a positive number!");
	if (width <= 0) error("Width must be a positive number!");
	if (frames <= 0) error("Frames must be a positive number!");

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// =================================================
	// 
	//					SHARED MEMORY
	// 
	// =================================================
	if (mode == 0 || mode == 1) {
		if (rank == 0) {
			printf("// ================================\n"
				"//\n"
				"//	Running Game on Shared Memory ...\n"
				"//	Threads		: %i\n"
				"//	Frames		: %i\n"
				"//	Field		: %ix%i\n"
				"//	Input  file	: %s\n"
				"//	Output file	: %s\n"
				"//\n"
				"// ================================\n",
				process_count,
				frames,
				height,
				width,
				input_field_filename,
				output_field_filename1);
		}
		
		SharedMemory(width, height, frames, input_field_filename, output_field_filename1, folder_name);
	}

	// =================================================
	// 
	//				DISTRIBUTED MEMORY
	// 
	// =================================================
	if (mode == 0 || mode == 2) {
		if (rank == 0) {
			printf("// ================================\n"
				"//\n"
				"//	Running Game on Distributed Memory ...\n"
				"//	Threads		: %i\n"
				"//	Frames		: %i\n"
				"//	Field		: %ix%i\n"
				"//	Input  file	: %s\n"
				"//	Output file	: %s\n"
				"//\n"
				"// ================================\n",
				process_count,
				frames,
				height,
				width,
				input_field_filename,
				output_field_filename2);
		}

		DistrMemory(width, height, frames, input_field_filename, output_field_filename2, folder_name);
	}
	MPI_Finalize();
	
	return 0;
}

// EOF