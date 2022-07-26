/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 25.04.2021
*
* Referenzen:
* Game Of Life:   https://github.com/jagregory/abrash-black-book (Chapter 17)
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

void setCell(cell Cell, unsigned int x, unsigned int y) {

    int increase_neighbor = 0x02;
    _helperCell(Cell, x, y, increase_neighbor);
}

void deleteCell(cell Cell, unsigned int x, unsigned int y) {

    int decrease_neighbor = -0x02;
    _helperCell(Cell, x, y, decrease_neighbor);
}

void nextGeneration(cell Cell) {

    unsigned int x, y, count;
    unsigned int h = Cell.height, w = Cell.width;
    unsigned char* cell_ptr;

    memcpy(Cell.temp_ptr, Cell.ptr, Cell.size);
    cell_ptr = Cell.temp_ptr;

    for (y = 0; y < h; y++) {

        x = 0;
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
    }
}

unsigned int seed;
void fillMap(cell Cell, unsigned int w, unsigned int h) {
    unsigned int x, y, init_length;

    // Get seed; random if 0
    seed = (unsigned)time(NULL);

    srand(seed);
    init_length = (w * h) / 2;
    do{
        x = rand() % (w - 1);
        y = rand() % (h - 1);
        setCell(Cell, x, y);
    } while (--init_length);
}

// =================================================
//
//                  GAME OF LIFE
// 
// =================================================

double GameSeriell(unsigned width, unsigned height, unsigned frames, char* input_field_filename, char* output_field_filename) {

    cell map;
    unsigned int size = width * height;
    char* cells = (char*)xmalloc(size);
    char* temp = (char*)xmalloc(size);
    
    map.width = width;
    map.height = height;
    map.size = size;
    map.ptr = cells;
    map.temp_ptr = temp;

    memset(cells, 0, size);
    
    life106_read_file_memory(input_field_filename, &map);

    clock_t start = clock();

    for (int i = 0; i < frames; i++) {
        nextGeneration(map);
    }

    clock_t end = clock();

    life106_save_file_memory(output_field_filename, &map);

    free(cells);
    free(temp);

    return (double)(end - start) / (double)CLOCKS_PER_SEC;
}
