/*************************************************************************
* Title		: HPC Game Of Life
* Author	: Dimitri Dening
* Created	: 18.06.2021
* License	: MIT License
*
*		Copyright (C) 2021 Dimitri Dening
*
*		Permission is hereby granted, free of charge, to any person obtaining a copy
*		of this software and associated documentation files (the "Software"), to deal
*		in the Software without restriction, including without limitation the rights
*		to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*		copies of the Software, and to permit persons to whom the Software is
*		furnished to do so, subject to the following conditions:
*
*		The above copyright notice and this permission notice shall be included in all
*		copies or substantial portions of the Software.
*
*		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*		IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*		FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*		LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*		OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*		SOFTWARE.
*
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* User defined headers */
#include "export.h"
#include "mem_optimized.h"
#include "utils.h"
#include "export.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

void mergeRows(char* curr_row, char* halo_row, unsigned int width) {

	unsigned int row;

	for (row = 0; row < width; row++) {
		*(curr_row + row) += (*(halo_row + row));
	}
}

int main(int argc, char** argv) {
	if (argc != 7) error("Arguments: <height> <width> <frames> <input.lif> <output.lif> <new_folder_name>");

	int process_count;
	int rank;
	int field_size[1];
	int mode;			// 0: both, 1: shared, 2: distributed

	double start_time;
	double end_time;
	double duration;
	double data[1];

	unsigned int width;
	unsigned int height;
	unsigned int frames;

	char* input_field_filename;
	char* output_field_filename;
	char* output_field_filename2;
	char* export_filename;
	char* folder_name;

	size_t array_size;

	width = atoi(argv[1]);
	height = atoi(argv[2]);
	frames = atoi(argv[3]);
	input_field_filename = argv[4];
	output_field_filename = argv[5];
	// export_filename = argv[6];
	folder_name = argv[6];

	if (height <= 0) error("Height must be a positive number!");
	if (width <= 0) error("Width must be a positive number!");
	if (frames <= 0) error("Frames must be a positive number!");

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// =================================================
	// 
	//					MULTI NODE
	// 
	// =================================================

	MPI_Comm MPI_COMM_NODE = MPI_COMM_NULL;

	/*
	* MPI_COMM_SPLIT_TYPE(comm, split_type, key, info, newcomm)
	*
	* IN comm communicator(handle)
	* IN split_type type of processes to be grouped together(integer)
	* IN key control of rank assignment(integer)
	* IN info info argument(handle)
	* OUT newcomm new communicator(handle)
	*/

	// key = 0    : Rank Reihenfolge beginnt im neuen Communicator wieder bei 0
	// key = rank : Rank wird wie im Original weitergeführt 
	MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &MPI_COMM_NODE);

	int new_rank;
	int new_process_count;
	MPI_Comm_size(MPI_COMM_NODE, &new_process_count);
	MPI_Comm_rank(MPI_COMM_NODE, &new_rank);

	// =================================================
	// 
	//				SHARED MEMORY SEGMENT
	// 
	// =================================================
	// 
	//				Create MPI Struct
	// 
	// =================================================
	int lengths[7] = { 1, 1, 1, 1, 1, 1, 1 };
	MPI_Datatype types[7] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_CHAR, MPI_CHAR };
	MPI_Datatype map_type;
	MPI_Aint displacements[7];

	// Calculate offsets
	displacements[0] = offsetof(cell, width);
	displacements[1] = offsetof(cell, width_0);
	displacements[2] = offsetof(cell, height);
	displacements[3] = offsetof(cell, height_0);
	displacements[4] = offsetof(cell, size);
	displacements[5] = offsetof(cell, ptr);
	displacements[6] = offsetof(cell, temp_ptr);

	//					n_items	block_lengths  offsets
	MPI_Type_create_struct(7, lengths, displacements, types, &map_type);
	MPI_Type_commit(&map_type);

	// =================================================
	// 
	//				ALLOCATE SHARED MEMORY
	// 
	// =================================================
	char* shptr;	// (OUT) address of local allocated window segment 
	MPI_Win shwin;	// (OUT) window object returned by the call (handle) 

	int fsize;
	fsize = width * height;
	if (new_rank == 0) {
		MPI_Win_allocate_shared(fsize * sizeof(char), fsize * sizeof(char), MPI_INFO_NULL, MPI_COMM_NODE, &shptr, &shwin);
	}
	else {
		MPI_Win_allocate_shared(0, fsize * sizeof(char), MPI_INFO_NULL, MPI_COMM_NODE, &shptr, &shwin);
	}

	MPI_Aint rsize;		// (OUT) size of the window segment (non-negative integer)
	int rdisp;			// (OUT) local unit size for displacements, in bytes (positive integer)
	char* rptr = NULL;	// (OUT) address for load/store access to window segment

	/*
	* Starts an RMA access epoch to all processes in win, with a lock type of
	* MPI_LOCK_SHARED. During the epoch, the calling process can access the window memory on
	* all processes in win by using RMA operations. A window locked with MPI_WIN_LOCK_ALL
	* must be unlocked with MPI_WIN_UNLOCK_ALL. This routine is not collective — the ALL
	* refers to a lock on all members of the group of the window.
	*/
	MPI_Win_lock_all(0, shwin);

	//				SHARED_MEM_WIN	RANK	SIZE_WIN_SEG		
	MPI_Win_shared_query(shwin, MPI_PROC_NULL, &rsize, &rdisp, &rptr); // rank : MPI_PROC_NULL oder 0 ?
	if (rptr == NULL || rsize != (fsize * sizeof(char))) {
		printf("rptr=%p rsize=%zu \n", rptr, (size_t)rsize);
		MPI_Abort(MPI_COMM_NODE, 1);
	}
	
	// =================================================
	// 
	//					Init Phase
	// 
	// =================================================
	cell m;
	cell pm;
	cell rmap;

	if (new_rank == 0) {

		m = GameMap_Init_Shrd(rptr, width, height, input_field_filename);
		// Verwende rank und nicht new_rank um Teilfeld zu bestimmen
		// Durch MPI_Comm_split_type fängt new_rank wieder bei 0.
		// Die Verwendung von new_rank würde hier dafür sorgen, dass nur die ersten n Teilfelder bestimmt werden. 
		pm = createMapObject(m, &rank, &process_count);		

		/*
		* The call MPI_WIN_SYNC synchronizes the private and public window copies of win.
		* For the purposes of synchronizing the private and public window, MPI_WIN_SYNC has the
		* effect of ending and reopening an access and exposure epoch on the window
		*/
		MPI_Win_sync(shwin);

		for (int j = 1; j < new_process_count; j++) {
			MPI_Send(&m, 1, map_type, j, 0, MPI_COMM_NODE);
		}
	}

	MPI_Barrier(MPI_COMM_NODE);		// Ist das überhaupt sinnvoll hier?

	if (new_rank != 0) {

		MPI_Win_shared_query(shwin, 0, &rsize, &rdisp, &rptr);
		MPI_Recv(&rmap, 1, map_type, 0, 0, MPI_COMM_NODE, MPI_STATUSES_IGNORE);

		/*
		* The call MPI_WIN_SYNC synchronizes the private and public window copies of win.
		* For the purposes of synchronizing the private and public window, MPI_WIN_SYNC has the
		* effect of ending and reopening an access and exposure epoch on the window
		*/
		MPI_Win_sync(shwin);

		cell map;
		m.width_0 = 0;
		m.height_0 = 0;
		m.width = rmap.width;
		m.height = rmap.height;
		m.size = rmap.size;
		m.ptr = rptr;
		m.temp_ptr = malloc(m.size * sizeof(char));

		// Verwende rank und nicht new_rank um Teilfeld zu bestimmen
		// Durch MPI_Comm_split_type fängt new_rank wieder bei 0.
		// Die Verwendung von new_rank würde hier dafür sorgen, dass nur die ersten n Teilfelder bestimmt werden. 
		pm = createMapObject(rmap, &rank, &process_count);
		pm.ptr = rptr;
		pm.temp_ptr = malloc(pm.size * sizeof(char));
	}

	MPI_Win_unlock_all(shwin);

	/*
	* Barrier Synchronization
	* If comm is an intracommunicator, MPI_BARRIER blocks the caller until all group members have called it.
	* The call returns at any process only after all group members have entered the call.
	*/
	// MPI_Win_sync(shwin);
	MPI_Barrier(MPI_COMM_NODE);

	// =================================================
	// 
	//	            DISTRIBUTED MEMORY
	// 
	// =================================================
	int prev_rank;
	int post_rank;

	char* recv_buffer_top;
	char* recv_buffer_bot;
	char* top_row;
	char* bot_row;
	char* halo_top;
	char* halo_bot;
	unsigned char* recv_buffer_im;

	recv_buffer_top = malloc(pm.width * sizeof(char));
	recv_buffer_bot = malloc(pm.width * sizeof(char));
	recv_buffer_im = malloc(pm.size * sizeof(char));
	top_row = (pm.ptr + (pm.width * pm.height_0));
	bot_row = (pm.ptr + (pm.width * pm.height));
	halo_top = pm.height_0 == 0 ? m.ptr + (m.width * (m.height - 1)) : m.ptr + (m.width * (pm.height_0 - 1));
	halo_bot = (pm.height + 1) >= m.height ? m.ptr : (pm.ptr + (pm.width * (pm.height + 1)));

	// funktioniert nur wenn jeder Node gleiche Anzahl an Kernen hat
	prev_rank = (rank == 0) ? process_count - new_process_count : rank - new_process_count; 
	if (prev_rank < 0) prev_rank = process_count - new_process_count;
	
	post_rank = (rank == (process_count - 1)) ? 0 : rank + new_process_count;
	if (post_rank >= process_count) post_rank = 0;

	char* halo_top_new;
	char* halo_bot_new;
	char* top_row_new;
	char* bot_row_new;
	int delta_h = pm.height - pm.height_0;

	halo_top_new = pm.height_0 == 0 ? m.ptr + (m.width * (m.height - 1)) : m.ptr + (pm.width * (pm.height_0 - 1)); 
	halo_bot_new = (pm.height_0 + (delta_h + 1) * new_process_count) >= m.height ? m.ptr : pm.ptr + (pm.width * (delta_h + 1) * new_process_count);

	top_row_new = (pm.ptr + (pm.width * pm.height_0));
	bot_row_new = pm.ptr + ((pm.width * (delta_h + 1) * new_process_count) - pm.width);

	
	// =================================================
	// 
	//			RANK0 to RANKn working on field
	// 
	// =================================================

	// if (rank == 0) start_time = MPI_Wtime();
	MPI_Request request;

	// Prepare parameters
	int tag_send = 0;
	int tag_recv = tag_send;

	double comm_start, comm_end;
	double calc_start, calc_end;
	double comm_time = 0, calc_time = 0;

	if (rank == 0) start_time = MPI_Wtime();

	for (int lo = 0; lo < frames; lo++) {

		if (new_rank == 0) {
			memset(halo_top_new, 0, m.width);
			memset(halo_bot_new, 0, m.width);
		}
		
		if (rank == 0) calc_start = MPI_Wtime();

		if (new_rank == 0) {
			MPI_Win_sync(shwin);
			GameMPI(m, pm, 1, MPI_COMM_NODE);		// start game in shared mode
		}

		if (new_rank != 0) {
			GameMPI(m, pm, 1, MPI_COMM_NODE);		// start game in shared mode
			MPI_Win_sync(shwin);
		}

		if (rank == 0) {
			calc_end = MPI_Wtime();
			calc_time += (calc_end - calc_start);
		}

		//if (process_count == new_process_count) {
		//	// KEIN SEND WEIL WIR AUF EINEM SYSTEM SIND
		//}

		/*
		* Barrier Synchronization
		* If comm is an intracommunicator, MPI_BARRIER blocks the caller until all group members have called it.
		* The call returns at any process only after all group members have entered the call.
		*/
		if (rank == 0) comm_start = MPI_Wtime();
		MPI_Barrier(MPI_COMM_NODE); // hier MPI_COMM_NODE verwenden ?

		if (new_rank == 0) {

				MPI_Sendrecv(halo_top_new, pm.width, MPI_CHAR, prev_rank, tag_send,
						     recv_buffer_top, pm.width, MPI_CHAR, post_rank, tag_recv, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				MPI_Sendrecv(halo_bot_new, pm.width, MPI_CHAR, post_rank, tag_send,
					recv_buffer_bot, pm.width, MPI_CHAR, prev_rank, tag_recv, MPI_COMM_WORLD, MPI_STATUS_IGNORE);			
		}

		MPI_Barrier(MPI_COMM_WORLD);

		if (rank == 0) {
			comm_end = MPI_Wtime();
			comm_time += (comm_end - comm_start);
		}

		if (new_rank == 0) {
			mergeRows(top_row_new, recv_buffer_bot, pm.width);
			mergeRows(bot_row_new, recv_buffer_top, pm.width);
		}

		// Dannach nochmal ein Sync für den geteilten Speicher nötig?
		MPI_Win_sync(shwin);
		MPI_Barrier(MPI_COMM_WORLD);
	}
	
	// =================================================
	// 
	//					Merge Image
	// 
	// =================================================
	MPI_Barrier(MPI_COMM_NODE);

	char* buffer;
	buffer = malloc(m.size * sizeof(char));

	if (rank == 0) {
		MPI_Gather(top_row, pm.size, MPI_CHAR, buffer, pm.size, MPI_CHAR, 0, MPI_COMM_WORLD);
	}
	else {
		MPI_Gather(top_row, pm.size, MPI_CHAR, NULL, 0, MPI_CHAR, 0, MPI_COMM_WORLD);
	}
	
	double comm_data[1] = { comm_time };
	double calc_data[1] = { calc_time };

	if (rank == 0) {
		m.ptr = buffer;
		end_time = MPI_Wtime();
		life106_save_file_memory(output_field_filename, &m);
		duration = end_time - start_time;
		data[0] = duration;
		field_size[0] = width;
		array_size = NELEMS(data);
		exportJson("multi-node", data, array_size, field_size, process_count, frames, folder_name);
		exportDataArray("multi-node_comm", comm_data, NELEMS(comm_data), process_count);
		exportDataArray("multi-node_calc", calc_data, NELEMS(calc_data), process_count);
	}

	MPI_Win_free(&shwin);
	MPI_Finalize();
	return 0;
}

// EOF