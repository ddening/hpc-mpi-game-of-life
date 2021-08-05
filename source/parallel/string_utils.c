#include "string_utils.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned string_count_splits(const char* string, const char* delimiters)
{
	unsigned string_length = strlen(string) + 1; // plus \0
	char* temp_string = malloc(sizeof(char) * string_length);
	if (temp_string == NULL) error("Can't allocate memory for temporary string!");
	unsigned errors = strcpy_s(temp_string, string_length, string);
	if (errors != 0) error("Can't copy to temporary string!");

	char* context = NULL;
	char* first_split = strtok_s(temp_string, delimiters, &context);
	if (first_split == NULL)
	{
		free(temp_string);
		return 0;
	}

	unsigned count = 1;
	while (strtok_s(NULL, delimiters, &context) != NULL) count++;

	free(temp_string);
	return count;
}

unsigned string_split(char* string, const char* delimiters, char*** splits)
{
	unsigned splits_count = string_count_splits(string, delimiters);

	if (splits_count == 0) error("String doesn't contain any delimiter!");

	*splits = malloc(sizeof(char*) * splits_count);
	if (*splits == NULL) error("Can't allocate splits memory!");

	// Find substrings between delimiters
	char* context = NULL;
	(*splits)[0] = strtok_s(string, delimiters, &context);
	for (int i = 1; i < splits_count; i++)
	{
		(*splits)[i] = strtok_s(NULL, delimiters, &context);
	}

	return splits_count;
}
