/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 11.05.2021
*
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* User defined headers */
#include "mem_optimized.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

/**
 * @brief Illustrates how to create an indexed MPI datatype.
 * 
 * Structure of a cell (short version for illustration):
 * - width	: int
 * - height	: int
 * - size	: int 
 * 
 * How to represent such a structure with an MPI struct:
 *
 *           +----------------- displacement for
 *           |        block 2: sizeof(int) + sizeof(int)
 *           |               (+ potential padding)
 *           |                         |
 *           +----- displacement for   |
 *           |    block 2: sizeof(int) |
 *           |   (+ potential padding) |
 *           |            |            |
 *  displacement for      |            |
 *    block 1: 0          |            |
 * (+ potential padding)  |            |
 *           |            |            |
 *           V            V            V
 *           +------------+------------+------------+
 *           |    width   |   height   |    size    |
 *           +------------+------------+------------+
 *            <----------> <----------> <---------->
 *               block 1      block 2      block 3
 *              1 MPI_INT    1 MPI_INT	  1 MPI_INT
 **/
void SharedMemory(unsigned w, unsigned h, unsigned frames, char* input_field_filename, char* output_field_filename, char* folder_name) {

	int process_count;
	int rank;
	int fsize;
	int field_size[1];

	double start_time;
	double end_time;
	double duration;
	double data[1];
	size_t array_size;

	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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
	//	            SHARED MEMORY
	// 
	// =================================================
	char* shptr;	// (OUT) address of local allocated window segment 
	MPI_Win shwin;	// (OUT) window object returned by the call (handle) 

	fsize = w * h;
	if (rank == 0) {			
		MPI_Win_allocate_shared(fsize * sizeof(char), fsize * sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &shptr, &shwin);
	}
	else {
		MPI_Win_allocate_shared(0, fsize * sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &shptr, &shwin);
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
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	// =================================================
	// 
	//					Game Of Life
	// 
	// =================================================
	cell m;
	cell pm;
	cell rmap;

	if (rank == 0) {

		m = GameMap_Init_Shrd(rptr, w, h, input_field_filename);
		pm = createMapObject(m, &rank, &process_count);

		/*
		* The call MPI_WIN_SYNC synchronizes the private and public window copies of win.
		* For the purposes of synchronizing the private and public window, MPI_WIN_SYNC has the
		* effect of ending and reopening an access and exposure epoch on the window
		*/
		MPI_Win_sync(shwin);

		for (int j = 1; j < process_count; j++) {
			MPI_Send(&m, 1, map_type, j, 0, MPI_COMM_WORLD);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank != 0) {

		MPI_Win_shared_query(shwin, 0, &rsize, &rdisp, &rptr);
		MPI_Recv(&rmap, 1, map_type, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

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
	MPI_Barrier(MPI_COMM_WORLD);

	// =================================================
	// 
	//			RANK0 to RANKn working on field
	// 
	// =================================================

	double comm_start, comm_end;
	double calc_start, calc_end;
	double comm_time = 0, calc_time = 0;

	if (rank == 0) start_time = MPI_Wtime();

	for (int lo = 0; lo < frames; lo++) {

		if (rank == 0) {
			calc_start = MPI_Wtime();
			MPI_Win_sync(shwin);
			GameMPI(m, pm, 1);		// start game in shared mode
			calc_end = MPI_Wtime();
			calc_time += (calc_end - calc_start);
		}

		if (rank != 0) {
			GameMPI(m, pm, 1);		// start game in shared mode
			MPI_Win_sync(shwin);
		}

		/*
		* Barrier Synchronization
		* If comm is an intracommunicator, MPI_BARRIER blocks the caller until all group members have called it.
		* The call returns at any process only after all group members have entered the call.
		*/
		comm_start = MPI_Wtime();
		MPI_Barrier(MPI_COMM_WORLD);
		comm_end = MPI_Wtime();
		comm_time += (comm_end - comm_start);
	}

	if (rank == 0) { 
		end_time = MPI_Wtime(); 
		life106_save_file_memory(output_field_filename, &m);
	}

	MPI_Win_free(&shwin);

	double comm_data[1] = { comm_time };
	double calc_data[1] = { calc_time };

	// printf("Rank: %i, Comm_time: %f, Calc_time: %f\n", rank, comm_time, calc_time);

	if (rank == 0) {
		duration = end_time - start_time;
		data[0] = duration;
		field_size[0] = w;
		array_size = NELEMS(data);
		exportJson("shrd", data, array_size, field_size, process_count, frames, folder_name);
		exportDataArray("shrd_comm", comm_data, NELEMS(comm_data), process_count);
		exportDataArray("shrd_calc", calc_data, NELEMS(calc_data), process_count);
	}
}