#include "life106.h"

#include "field.h"
#include "utils.h"
#include "string_utils.h"
#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem_optimized.h"

typedef struct
{
	int x;
	int y;
} coordinate;

void life106_read_file(const char* filename, field_t* field)
{
	// delete everything in the field
	for_yx(field->height, field->width)
	{
		field->rows[y][x] = 0;
	}
	//
	char* file_content = read_file(filename);
	char** lines = NULL;
	unsigned lines_count = string_split(file_content, "\r\n", &lines);

	// parse coordinates
	unsigned coordinates_count = 0;
	coordinate* coordinates = malloc(sizeof(coordinate) * lines_count);
	if (coordinates == NULL) error("Can't allocate memory for coordinates!");

	for_i(i, lines_count)
	{
		char* current_line = lines[i];
		if (current_line[0] == '#') continue; // skip comments

		// split coordinate
		char** coordinate_splits;
		unsigned coordinate_splits_count = string_split(current_line, " ", &coordinate_splits);
		if (coordinate_splits_count != 2) error("Invalid Life 1.06 file!");

		// parse coordinate
		int x = strtol(coordinate_splits[0], NULL, 0);
		int y = strtol(coordinate_splits[1], NULL, 0);

		coordinates[coordinates_count++] = (coordinate){ .x = x, .y = y };
	}

	// read coordinates to field
	unsigned offset_x = field->width / 2;
	unsigned offset_y = field->height / 2;
	for_i(i, coordinates_count)
	{
		coordinate current_coordinate = coordinates[i];

		// move coordinate by half of the field dimensions
		int y = current_coordinate.y + offset_y;
		int x = current_coordinate.x + offset_x;

		// skip coordinate if it is outside the field
		if (y < 0 || y >= field->height || x < 0 || x >= field->width) continue;

		field->rows[y][x] = 1;
	}
	
	free(file_content);
}

void life106_save_file(const char* filename, field_t* field)
{
	FILE* file = NULL;
	errno_t error_number = fopen_s(&file, filename, "wb");
	if (error_number != 0 || file == NULL) error("Can't open file!");

	fprintf(file, "#Life 1.06\r\n");

	for_yx(field->height, field->width)
	{
		if (field->rows[y][x] == 1)
		{
			fprintf(file, "%i %i\r\n", x, y);
		}
	}
	fclose(file);
}

// =================================================
//
//						MEMORY
// 
// =================================================

void life106_read_file_memory(const char* filename, cell* field){

	char* file_content = read_file(filename);
	char** lines = NULL;
	unsigned lines_count = string_split(file_content, "\r\n", &lines);

	// parse coordinates
	unsigned coordinates_count = 0;
	coordinate* coordinates = malloc(sizeof(coordinate) * lines_count);
	if (coordinates == NULL) error("Can't allocate memory for coordinates!");

	for_i(i, lines_count)
	{
		char* current_line = lines[i];
		if (current_line[0] == '#') continue; // skip comments

		// split coordinate
		char** coordinate_splits;
		unsigned coordinate_splits_count = string_split(current_line, " ", &coordinate_splits);
		if (coordinate_splits_count != 2) error("Invalid Life 1.06 file!");

		// parse coordinate
		int x = strtol(coordinate_splits[0], NULL, 0);
		int y = strtol(coordinate_splits[1], NULL, 0);

		coordinates[coordinates_count++] = (coordinate){ .x = x, .y = y };
	}

	// read coordinates to field
	unsigned offset_x = field->width / 2;
	unsigned offset_y = field->height / 2;
	for_i(i, coordinates_count)
	{
		coordinate current_coordinate = coordinates[i];

		// move coordinate by half of the field dimensions
		int y = current_coordinate.y + offset_y;
		int x = current_coordinate.x + offset_x;

		// skip coordinate if it is outside the field
		if (y < 0 || y >= field->height || x < 0 || x >= field->width) continue;
		setCell(*field, x, y);
	}

	free(file_content);
}

void life106_save_file_memory(const char* filename, cell* field){
	FILE* file = NULL;
	errno_t error_number = fopen_s(&file, filename, "wb");
	if (error_number != 0 || file == NULL) error("Can't open file!");

	int x = 0;
	int y = 0;

	fprintf(file, "#Life 1.06\r\n");

	for (int i = 0; i < field->size; i++) {

		if (x >= field->width) {
			x = 0;
			y++;
		}

		if (*(field->ptr + i) & 0x01) {
			fprintf(file, "%i %i\r\n", x, y);
		}

		x++;
	}

	fclose(file);
}