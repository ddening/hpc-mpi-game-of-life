#include "field.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

field_t* field_new(unsigned height, unsigned width)
{
	field_t* field = malloc(sizeof(field_t));
	if (field == NULL) return NULL;

	field->height = height;
	field->width = width;

	field->rows = malloc(sizeof(unsigned*) * height);
	if (field->rows == NULL)
	{
		free(field);
		return NULL;
	}
	memset(field->rows, 0, height);

	for_i(y, height)
	{
		field->rows[y] = malloc(sizeof(unsigned) * width);
		if (field->rows[y] == NULL)
		{
			field_delete(field);
			return NULL;
		}
	}

	return field;
}

void field_delete(field_t* field)
{
	for_i(y, field->height)
	{
		if (field->rows[y] != NULL)
		{
			free(field->rows[y]);
			field->rows[y] = NULL;
		}
	}

	free(field->rows);
	free(field);
}
