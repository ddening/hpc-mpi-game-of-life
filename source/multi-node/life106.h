#pragma once

#include "field.h"
#include "mem_optimized.h"

// =================================================
//
//              FUNCTION PROTOTYPES
// 
// =================================================

void life106_read_file(const char* filename, field_t* field);
void life106_save_file(const char* filename, field_t* field);
void life106_read_file_memory(const char* filename, cell* field);
void life106_save_file_memory(const char* filename, cell* field);
