/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 08.05.2021
*
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* User defined headers */
#include "mem_optimized.h"
#include "life106.h"

void* xmalloc(size_t bytes) {

    void* buffer = malloc(bytes);
    if (buffer == NULL) {
        fprintf(stderr, "Allocation error...\n");
        exit(EXIT_FAILURE);
    }

    return buffer;
}

void _helperCell(cell Cell, unsigned int x, unsigned int y, signed int neighbor) {

    int w = Cell.width, h = Cell.height, size = Cell.size;
    int xoleft, xoright, yoabove, yobelow;
    int offset = (y * w) + x;
    unsigned char* cell_ptr = Cell.ptr + offset;

    // Calculate the offsets to the eight neighboring cells,
    // accounting for wrapping around at the edges of the cell map
    xoleft = (x == 0) ? w - 1 : -1;
    xoright = (x == (w - 1)) ? -(w - 1) : 1;
    yoabove = (y == 0) ? size - w : -w;
    yobelow = (y == (h - 1)) ? -(size - w) : w;

    if (neighbor < 0) {
        *(cell_ptr) &= ~0x01;   // Set first bit to 0
    }
    else {
        *(cell_ptr) |= 0x01;    // Set first bit to 1
    }

    // Change successive bits for neighbour counts
    *(cell_ptr + yoabove + xoleft) += neighbor;
    *(cell_ptr + yoabove) += neighbor;
    *(cell_ptr + yoabove + xoright) += neighbor;
    *(cell_ptr + xoleft) += neighbor;
    *(cell_ptr + xoright) += neighbor;
    *(cell_ptr + yobelow + xoleft) += neighbor;
    *(cell_ptr + yobelow) += neighbor;
    *(cell_ptr + yobelow + xoright) += neighbor;
}

void setCell(cell* Cell, unsigned int x, unsigned int y) {

    int increase_neighbor = 0x02;
    _helperCell(*Cell, x, y, increase_neighbor);
}

void deleteCell(cell* Cell, unsigned int x, unsigned int y) {

    int decrease_neighbor = -0x02;
    _helperCell(*Cell, x, y, decrease_neighbor);
}

void nextGeneration(cell* Cell, cell* pCell, int mode, double* ptr_mem) {

    int x, y, count, w_diff, w_diff_0, h_diff;
    int h0 = pCell->height_0;
    int w0 = pCell->width_0;
    int h = pCell->height;
    int w = pCell->width;     
    unsigned char* cell_ptr;

    w_diff = Cell->width - pCell->width;
    w_diff_0 = Cell->width - pCell->width_0; // 0 - x : immer negativ? bzw. wird immer zu 0 in unserem Beispiel
    h_diff = Cell->height - pCell->height;

    double start_time, end_time, duration;
    start_time = MPI_Wtime();
    memcpy(Cell->temp_ptr, Cell->ptr, Cell->size);
    end_time = MPI_Wtime();

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    duration = end_time - start_time;
    *ptr_mem += duration;
    // printf("rank: %i, mem copy: %lf\n", rank, *ptr_mem);

    if (mode == 1) MPI_Barrier(MPI_COMM_WORLD);
    
    cell_ptr = Cell->temp_ptr;
    cell_ptr += (h0 * Cell->width); // wenn Anfangswert von y != 0 muss ptr erst noch verschoben werden
    cell_ptr += w0; 
    
    for (y = h0; y <= h; y++) {
 
        x = w0; // x = 0;
        do {
            // Zero bytes are off and have no neighbours so skip them...
            while (*cell_ptr == 0) {
                cell_ptr++; // Advance to the next cell
                // If all cells in row are off with no neighbours go to next row
                if (++x >= w) goto RowDone;
            }
            
            // Remaining cells are either on or have neighbours
            count = *cell_ptr >> 1; // # of neighboring on-cells
            if (*cell_ptr & 0x01) {

                // On cell must turn off if not 2 or 3 neighbours
                if ((count != 2) && (count != 3)) {
                    deleteCell(Cell, x, y);
                }
            }
            else {

                // Off cell must turn on if 3 neighbours
                if (count == 3) {
                    setCell(Cell, x, y);
                }
            }
            // Advance to the next cell byte
            cell_ptr++;
        } while (++x < w);
    RowDone:;
    cell_ptr += w_diff + w0;     // wenn w von 0 -> kleiner w läuft UND wenn w von xyz -> w_ende läuft
    }
}

unsigned int seed;
void fillMap(cell* Cell, unsigned int w, unsigned int h) {
    unsigned int x, y, init_length;

    // Get seed; random if 0
    seed = (unsigned)time(NULL);

    srand(seed);
    init_length = (w * h) / 2;
    do {
        x = rand() % (w - 1);
        y = rand() % (h - 1);
        setCell(Cell, x, y);
    } while (--init_length);
}

cell GameMap_Init_Shrd(char* rptr, unsigned width, unsigned height, char* input_field_filename) {

    unsigned int size = width * height;
    char* temp = (char*)xmalloc(size);

    cell map;
    map.width_0 = 0;
    map.height_0 = 0;
    map.width = width;
    map.height = height;
    map.size = size;
    map.ptr = rptr;
    map.temp_ptr = temp;

    if (rptr != NULL) {
        memset(rptr, 0, size);
    }
    
    life106_read_file_memory(input_field_filename, &map);

    return map;
}

cell GameMap_Init_Distr(unsigned width, unsigned height, char* input_field_filename) {

    unsigned int size = width * height;
    char* cells = (char*)xmalloc(size);
    char* temp = (char*)xmalloc(size);

    cell map;
    map.width_0 = 0;
    map.height_0 = 0;
    map.width = width;
    map.height = height;
    map.size = size;
    map.ptr = cells;
    map.temp_ptr = temp;
    memset(cells, 0, size);

    life106_read_file_memory(input_field_filename, &map);

    return map;
}

cell createMapObject(cell map, unsigned int* rank, unsigned int* process_count) {

    unsigned int num_procs;
    unsigned int tasks_per_proc;
    unsigned int full_rows_perc_proc;
    unsigned int remaining_rows;
    unsigned int pid;

    pid = (*rank + 1);
    num_procs = *process_count;
    tasks_per_proc = map.size / num_procs;
    full_rows_perc_proc = tasks_per_proc / map.width;
    remaining_rows = tasks_per_proc % map.width;

    cell pmap;
    pmap.width_0 = 0;
    pmap.height_0 = ((pid - 1) * full_rows_perc_proc);
    pmap.width = map.width;
    pmap.height = (pid * full_rows_perc_proc) - 1; 
    pmap.size = (pmap.width * (pmap.height + 1) - pmap.width * pmap.height_0);
    pmap.ptr = map.ptr;
    pmap.temp_ptr = map.temp_ptr;

    if (*rank == *process_count - 1) pmap.height = map.height - 1;

    return pmap;
}

void GameMap_Release(cell map) {
    free(map.ptr);
    free(map.temp_ptr);
}

// =================================================
//
//                  GAME OF LIFE
// 
// =================================================

/*
* Args:
*       mode:   0 -> distributed
*               1 -> shared
*/
void GameMPI(cell map, cell pmap, int mode, double* ptr_mem) {
    nextGeneration(&map, &pmap, mode, ptr_mem);
}
