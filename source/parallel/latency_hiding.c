/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 19.06.2021
*
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* User defined headers */
#include "mem_optimized.h"
#include "latency_hiding.h"
#include "life106.h"

void mergeRowsLatencyHiding(char* curr_row, char* halo_row, unsigned int width) {

    unsigned int row;

    for (row = 0; row < width; row++) {
        *(curr_row + row) += (*(halo_row + row));
    }
}

void nextGenerationLatencyHiding(cell* Cell, cell* pCell, int mode, double* comm_time_ptr, double* calc_time_ptr) {
  
    int prev_rank;
    int post_rank;
    int process_count;
    int rank;

    double comm_start, comm_end;
    double calc_start, calc_end;

    char* top_row;
    char* bot_row;
    char* halo_top;
    char* halo_bot;
    char* recv_buffer_top;
    char* recv_buffer_bot;

    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    prev_rank = (rank == 0) ? process_count - 1 : rank - 1;
    post_rank = (rank == (process_count - 1)) ? 0 : rank + 1;
    top_row = (pCell->ptr + (pCell->width * pCell->height_0));
    bot_row = (pCell->ptr + (pCell->width * pCell->height));
    halo_top = pCell->height_0 == 0 ? Cell->ptr + (Cell->width * (Cell->height - 1)) : Cell->ptr + (Cell->width * (pCell->height_0 - 1));
    halo_bot = (pCell->height + 1) >= Cell->height ? Cell->ptr : (pCell->ptr + (pCell->width * (pCell->height + 1)));
    recv_buffer_top = malloc(pCell->width * sizeof(char));
    recv_buffer_bot = malloc(pCell->width * sizeof(char));

    memset(halo_top, 0, pCell->width);
    memset(halo_bot, 0, pCell->width);
    memset(recv_buffer_top, 0, pCell->width);
    memset(recv_buffer_bot, 0, pCell->width);

    int x, y, count, w_diff, w_diff_0, h_diff;
    int h0 = pCell->height_0;
    int w0 = pCell->width_0;
    int h = pCell->height;
    int w = pCell->width;
    unsigned char* cell_ptr;

    w_diff = Cell->width - pCell->width;
    w_diff_0 = Cell->width - pCell->width_0; // 0 - x : immer negativ? bzw. wird immer zu 0 in unserem Beispiel
    h_diff = Cell->height - pCell->height;

    memcpy(Cell->temp_ptr, Cell->ptr, Cell->size);

    if (mode == 1) MPI_Barrier(MPI_COMM_WORLD);

    cell_ptr = Cell->temp_ptr;
    cell_ptr += (h0 * Cell->width); // wenn Anfangswert von y != 0 muss ptr erst noch verschoben werden
    cell_ptr += w0;

    MPI_Request request1;
    MPI_Request request2;
    MPI_Request request_send;

    for (y = h0; y <= h; y++) {

        if (y == h0) { 
            MPI_Irecv(recv_buffer_top, pCell->width, MPI_CHAR, post_rank, 0, MPI_COMM_WORLD, &request1);      
        }
        if (y == h) {   
            MPI_Irecv(recv_buffer_bot, pCell->width, MPI_CHAR, prev_rank, 0, MPI_COMM_WORLD, &request2); 
        }  
        
        if (rank == 0) calc_start = MPI_Wtime();

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

        if (rank == 0) {
            calc_end = MPI_Wtime();
            *calc_time_ptr += (calc_end - calc_start);
        }

        if (y == h0) {
            MPI_Isend(halo_top, pCell->width, MPI_CHAR, prev_rank, 0, MPI_COMM_WORLD, &request_send);
            if (rank == 0) comm_start = MPI_Wtime();
            MPI_Wait(&request1, MPI_STATUS_IGNORE); // Wait for the MPI_Recv to complete.
            if (rank == 0) {
                comm_end = MPI_Wtime();
                *comm_time_ptr += (comm_end - comm_start);
            }
            mergeRowsLatencyHiding(bot_row, recv_buffer_top, pCell->width);
        }

        if (y == h) {
            MPI_Isend(halo_bot, pCell->width, MPI_CHAR, post_rank, 0, MPI_COMM_WORLD, &request_send);  
            if (rank == 0) comm_start = MPI_Wtime();
            MPI_Wait(&request2, MPI_STATUS_IGNORE); // Wait for the MPI_Recv to complete.
            if (rank == 0) {
                comm_end = MPI_Wtime();
                *comm_time_ptr += (comm_end - comm_start);
            }
            mergeRowsLatencyHiding(top_row, recv_buffer_bot, pCell->width);
        }       
    } 
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
void GameMPILatencyHiding(cell map, cell pmap, int mode, double* comm_time_ptr, double* calc_time_ptr) {
    nextGenerationLatencyHiding(&map, &pmap, mode, comm_time_ptr, calc_time_ptr);
}
