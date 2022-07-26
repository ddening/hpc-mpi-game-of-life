#pragma once

typedef struct
{
	unsigned height;
	unsigned width;
	unsigned** rows;
} field_t;

field_t* field_new(unsigned height, unsigned width);
void field_delete(field_t* field);
