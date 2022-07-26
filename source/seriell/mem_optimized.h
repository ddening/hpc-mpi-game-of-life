#pragma once

// =================================================
//
//                  STRUCTURES
// 
// =================================================

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int size;
    unsigned char* ptr;
    unsigned char* temp_ptr;
} cell;

// =================================================
//
//              FUNCTION PROTOTYPES
// 
// =================================================

double GameSeriell(unsigned width, unsigned height, unsigned frames, char* input_field_filename, char* output_field_filename);
void setCell(cell Cell, unsigned int x, unsigned int y);
void deleteCell(cell Cell, unsigned int x, unsigned int y);