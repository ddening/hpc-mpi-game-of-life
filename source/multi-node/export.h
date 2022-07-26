#pragma once

// =================================================
//
//              FUNCTION PROTOTYPES
// 
// =================================================

void exportDataArray(char* filename, double data[], size_t array_length, int process_count);
void exportJson(char* filename, double data[], size_t array_length, int field_size[], int process_count, int frames, char* folder_name);