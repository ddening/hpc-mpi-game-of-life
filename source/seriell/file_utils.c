#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>

char* read_file(const char* filename)
{
	FILE* file = NULL;
	errno_t error_number = fopen_s(&file, filename, "rb");
	if (error_number != 0 || file == NULL) error("Can't open file!");

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* file_content = malloc(sizeof(char) * (file_size + 1));
	if (file_content == NULL) error("Can't allocate memory for file content!");

	file_content[file_size] = '\0'; // Null-terminate string manually because fread doesn't do this

	fread(file_content, sizeof(char), file_size, file);
	fclose(file);

	return file_content;
}

void save_file(const char* filename, const char* content)
{
	FILE* file = NULL;
	errno_t error_number = fopen_s(&file, filename, "w");
	if (error_number != 0 || file == NULL) error("Can't open file!");

	fputs(content, file);

	fclose(file);
}
