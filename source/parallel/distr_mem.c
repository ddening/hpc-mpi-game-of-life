/********************************************************************
*
* Titel : HPC Game Of Life
* Author: Dimitri Dening
* Date  : 21.05.2021
*
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mem_optimized.h"
#include "latency_hiding.h"
#include "export.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

void mergeRows(char* curr_row, char* halo_row, unsigned int width) {

	unsigned int row;

	for (row = 0; row < width; row++) {
		*(curr_row + row) += (*(halo_row + row));
	}
}

void DistrMemory(unsigned w, unsigned h, unsigned frames, char* input_field_filename, char* output_field_filename, char* folder_name) {

	int process_count;
	int rank;
	int prev_rank;
	int post_rank;
	int field_size[1];
	// int recv_temp;

	double start_time;
	double end_time;
	double duration;
	double data[1];
	size_t array_size;

	char* recv_buffer_top;
	char* recv_buffer_bot;
	unsigned char* recv_buffer_im;
	char* top_row;
	char* bot_row;
	char* halo_top;
	char* halo_bot;

	cell m;
	cell pm;

	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// =================================================
	// 
	//                DISTRIBUTED MEMORY
	// 
	// =================================================
	m = GameMap_Init_Distr(w, h, input_field_filename);
	pm = createMapObject(m, &rank, &process_count);

	recv_buffer_top = malloc(pm.width * sizeof(char));
	recv_buffer_bot = malloc(pm.width * sizeof(char));

	// =================================================
	// 
	//          RANK0 to RANKn working on field
	// 
	// =================================================
	prev_rank = (rank == 0) ? process_count - 1 : rank - 1;
	post_rank = (rank == (process_count - 1)) ? 0 : rank + 1;

	top_row = (pm.ptr + (pm.width * pm.height_0));
	bot_row = (pm.ptr + (pm.width * pm.height ));

	halo_top = pm.height_0 == 0 ? m.ptr + (m.width * (m.height - 1)) : m.ptr + (m.width * (pm.height_0 - 1));
	halo_bot =  (pm.height + 1) >= m.height ? m.ptr : (pm.ptr + (pm.width * (pm.height + 1)));

	double comm_start, comm_end;
	double calc_start, calc_end;
	double comm_time = 0, calc_time = 0;
	double* comm_time_ptr = &comm_time;
	double* calc_time_ptr = &calc_time;

	double mem = 0;
	double* ptr_mem = &mem;
	
	if (rank == 0) start_time = MPI_Wtime();
	
	for (unsigned int lo = 0; lo < frames; lo++) {

		memset(halo_top, 0, m.width);
		memset(halo_bot, 0, m.width);

		if (rank == 0) calc_start = MPI_Wtime();
		GameMPI(m, pm, 0, ptr_mem); // start game in distributed mode
		if (rank == 0) {
			calc_end = MPI_Wtime();
			calc_time += (calc_end - calc_start);
		}

		comm_start = MPI_Wtime();

		MPI_Sendrecv(halo_top, pm.width, MPI_CHAR, prev_rank, 0,
			recv_buffer_top, pm.width, MPI_CHAR, post_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		MPI_Sendrecv(halo_bot, pm.width, MPI_CHAR, post_rank, 0,
			recv_buffer_bot, pm.width, MPI_CHAR, prev_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		comm_end = MPI_Wtime();
		comm_time += (comm_end - comm_start);

		mergeRows(bot_row, recv_buffer_top, pm.width);
		mergeRows(top_row, recv_buffer_bot, pm.width);
	}
	
	// =================================================
	// 
	//                  Merge Image
	// 
	// =================================================
	char* buffer;
	char* temp_buffer;
	buffer = malloc(m.size * sizeof(char));

	if (rank == 0) {
		MPI_Gather(top_row, pm.size, MPI_CHAR, buffer, pm.size, MPI_CHAR, 0, MPI_COMM_WORLD);
	}
	else{
		MPI_Gather(top_row, pm.size, MPI_CHAR, NULL, 0, MPI_CHAR, 0, MPI_COMM_WORLD);
	}

	if (rank == 0) {
		temp_buffer = m.ptr;
		m.ptr = buffer;
		end_time = MPI_Wtime();
		life106_save_file_memory(output_field_filename, &m);
		free(temp_buffer);
	}

	free(m.ptr);
	free(m.temp_ptr);
	free(recv_buffer_bot);
	free(recv_buffer_top);
	

	double comm_data[1] = { comm_time };
	double calc_data[1] = { calc_time };
	double mem_data[1] = { mem };

	MPI_Barrier(MPI_COMM_WORLD);

	duration = end_time - start_time;
	data[0] = duration;
	field_size[0] = w;
	array_size = NELEMS(data);
	exportJson("dstrb", data, array_size, field_size, process_count, frames, folder_name);

	if (rank == 0) {
		exportDataArray("dstrb_comm", comm_data, NELEMS(comm_data), process_count);
		exportDataArray("dstrb_calc", calc_data, NELEMS(calc_data), process_count);
	}	

	char str[12];
	char* name = malloc(50);
	sprintf_s(str, 12, "%i", rank);

	name = strcat(strcat(name, "dstrb_mem_rank_"), str);
	exportDataArray(name, mem_data, NELEMS(mem_data), process_count);
}
