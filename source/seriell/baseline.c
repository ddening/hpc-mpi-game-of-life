#include "string_utils.h"
#include "field.h"
#include "life106.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void show(field_t* field)
{
	printf("\033[H");
	for_i(y, field->height)
	{
		for_i(x, field->width) printf(field->rows[y][x] ? "\033[07m  \033[m" : "  ");
		printf("\033[E");
	}
	fflush(stdout);
}

void evolve(field_t** field_pointer)
{
	field_t* field = *field_pointer;
	field_t* new_field = field_new(field->height, field->width);
	if (new_field == NULL) error("Can't allocate new field!");

	for_yx(field->height, field->width)
	{
		int n = 0;
		for (int y1 = y - 1; y1 <= y + 1; y1++)
			for (int x1 = x - 1; x1 <= x + 1; x1++)
				if (field->rows[(y1 + field->height) % field->height][(x1 + field->width) % field->width])
					n++;

		if (field->rows[y][x]) n--;
		new_field->rows[y][x] = (n == 3 || (n == 2 && field->rows[y][x]));
	}

	*field_pointer = new_field;
	field_delete(field);
}

double BaseGame(unsigned width, unsigned height, unsigned frames, char* input_field_filename, char* output_field_filename){

	field_t* field = field_new(height, width);
	if (field == NULL) error("Can't allocate field!");
	life106_read_file(input_field_filename, field);

	clock_t start_time = clock();
	for_i(i, frames)
	{
		evolve(&field);
	}
	clock_t end_time = clock();

	life106_save_file(output_field_filename, field);

	field_delete(field);
	field = NULL;

	return (double)(end_time - start_time) / (double)CLOCKS_PER_SEC;
}
