#pragma once

#include <mpi.h>

// =================================================
//
//                  STRUCTURES
// 
// =================================================

typedef struct {
    unsigned int width;
    unsigned int width_0;
    unsigned int height; 
    unsigned int height_0;
    unsigned int size;
    unsigned char* ptr;
    unsigned char* temp_ptr;
} cell;

// =================================================
//
//              FUNCTION PROTOTYPES
// 
// =================================================

void GameMPI(cell map, cell pmap, int mode, MPI_Comm MPI_COMM_NODE);
void GameMap_Release(cell map);
cell GameMap_Init_Shrd(char* rptr, unsigned width, unsigned height, char* input_field_filename);
cell GameMap_Init_Distr(unsigned width, unsigned height, char* input_field_filename);
cell createMapObject(cell map, unsigned int* rank, unsigned int* process_count);