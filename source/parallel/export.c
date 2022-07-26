/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 08.05.2021
*
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* time_t, time, ctime */
// #include <direct.h> // _getcwd

void exportDataArray(char* filename, double data[], size_t array_length, int process_count) {

	FILE* fptr;
	errno_t err_file, err_time;

	time_t now;
	struct tm tm;
	now = time(0);								

	char buffer[256], str[80];
	memset(str, 0, 80);

	err_time = localtime_s(&tm, &now);
	sprintf_s(buffer, sizeof(buffer), "_%04d_%02d_%02d_%02d_%02d_%02d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	err_file = fopen_s(&fptr, strcat(strcat(str, filename), buffer), "w");

	if (!err_file) {
		for (int idx = 0; idx < array_length; idx++) {
			fprintf(fptr, "%lf %i\n", data[idx], process_count);
		}
		fclose(fptr);
		printf("Data was exported into file %s\n", str);
	}
	else {
		printf("File was not opened\n");
	}
}

void exportJson(char* filename, double data[], size_t array_length, int field_size[], int process_count, int frames, char* folder_name) {

	FILE* fptr;
	errno_t err_file, err_time;

	time_t now;
	struct tm tm;
	now = time(0);							

	char buffer[256], str[80];
	memset(str, 0, 80);

	int check;
	char* mybuffer;
	char* new_folder_name = malloc(50);
	memset(new_folder_name, 0, 50);

	new_folder_name = strcat(strcat(strcat(new_folder_name, filename), "_"), folder_name);
	check = _mkdir(new_folder_name);

	// check if directory is created or not
	if (!check)
		printf("New Folder %s created\n", new_folder_name);
	else {
		printf("Unable to create directory\n");
		// exit(1);
	}

	_chdir(new_folder_name);

	err_time = localtime_s(&tm, &now);
	sprintf_s(buffer, sizeof(buffer), "_%04d_%02d_%02d_%02d_%02d_%02d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	err_file = fopen_s(&fptr, strcat(strcat(str, filename), buffer), "ab+");

	if (!err_file) {
		fprintf(fptr, "{");
		fprintf(fptr, "\"method\" : \"%s\", ", filename);
		fprintf(fptr, "\"threads\" : %i, ", process_count);
		fprintf(fptr, "\"frames\" : %i, ", frames);
		for (int idx = 0; idx < array_length; idx++) {
			fprintf(fptr, "\"size\" : %i, ", field_size[idx]);
			fprintf(fptr, "\"time\" : %lf, ", data[idx]);
		}
		fprintf(fptr, "\"notes\" : \"time in seconds\"");
		fprintf(fptr, "}");
		fclose(fptr);
		printf("Data was exported into file %s\n", str);
	}
	else {
		printf("File was not opened\n");
	}
}