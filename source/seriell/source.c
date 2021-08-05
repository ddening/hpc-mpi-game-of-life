/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 25.04.2021
*
*********************************************************************/

#include <stdio.h>

/* User defined headers */
#include "baseline.h"
#include "mem_optimized.h"
#include "export.h"
#include "utils.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

int main(int argc, char** argv) {
	if (argc != 9) error("Arguments: <height> <width> <frames> <input.lif> <output1.lif> <output2.lif> <mode> <new_folder_name>");

	int process_count = 1;
	int field_size[1];
	int mode;			// 0: both, 1: baseline, 2: optmzd
	
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
	// export_filename1 = argv[7];
	// export_filename2 = argv[8];
	mode = atoi(argv[7]);
	folder_name = argv[8];

	if (height <= 0) error("Height must be a positive number!");
	if (width <= 0) error("Width must be a positive number!");
	if (frames <= 0) error("Frames must be a positive number!");

	// =================================================
	// 
	//	            Base Game (Referenz)
	// 
	// =================================================
	if (mode == 0 || mode == 1) {
		printf("// ================================\n"
			"//\n"
			"//	Running Base Game ...\n"
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

		duration = BaseGame(width, height, frames, input_field_filename, output_field_filename1);

		printf("// \\\\     //\n");
		printf("//  \\\\   //\n");
		printf("//   \\\\_// Duration: %f seconds\n", duration);
		printf("\n");

		data[0] = duration;
		field_size[0] = width;
		array_size = NELEMS(data);
		exportJson("seriell", data, array_size, field_size, process_count, frames, folder_name);
	}
	// =================================================
	// 
	//	            Optimized Version
	// 
	// =================================================
	if (mode == 0 || mode == 2) {
		printf("// ================================\n"
			"//\n"
			"//	Running Optimized Game ...\n"
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

		duration = GameSeriell(width, height, frames, input_field_filename, output_field_filename2);

		printf("// \\\\     //\n");
		printf("//  \\\\   //\n");
		printf("//   \\\\_// Duration: %f seconds\n", duration);
		printf("\n");

		data[0] = duration;
		field_size[0] = width;
		array_size = NELEMS(data);
		exportJson("optmzd", data, array_size, field_size, process_count, frames, folder_name);
	}

	return 0;
}

// EOF