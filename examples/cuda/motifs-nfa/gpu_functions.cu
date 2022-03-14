// Original work:
// Copyright (C) 2010  
// Pierluigi Rolando (pierluigi.rolando@polito.it)
// Netgroup - DAUIN - Politecnico di Torino
//
// Niccolo' Cascarano (niccolo.cascarano@polito.it)
// Netgroup - DAUIN - Politecnico di Torino
//
// Modified work:
// Copyright (C) 2017  
// Vinh Dang (vqd8a@virginia.edu)
// University of Virginia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef DEVICE_EMU_DBG
#include <assert.h>
#include <stdio.h>
#endif

#include "common.h"
#include "gpu_functions.h"

#define myId		    threadIdx.x
#define thread_count    blockDim.x
#define nstreams        gridDim.x

extern __shared__ ST_BLOCK shared_base[];
__global__ void nfa_kernel(	st_t *nfa_tables,
							st_t *src_tables,
							unsigned int *input_transition_tables,
							symbol_fetch *input,
							unsigned long *cur_size_vec,
							ST_BLOCK *_svs,// ST_BLOCK is just a fancy name for a chunk of 32-bit unsigned data
							unsigned int *st_vec_lengths,
							ST_BLOCK *persistents,
							unsigned int *match_count, match_type *match_array, unsigned int match_vec_size,
							unsigned int *accum_nfa_table_lengths, unsigned int *accum_offset_table_lengths, unsigned int *accum_state_vector_lengths){
	
	__shared__ unsigned int shr_match_count;//Note: initializing is not allowed for shared variable
	shr_match_count = 0;
	
	unsigned int tmp_match_count;
	
	match_type tmp_match;
	
	// cur_size is the input string length
	size_t cur_size = (blockIdx.x == 0 ? cur_size_vec[blockIdx.x] : cur_size_vec[blockIdx.x] - cur_size_vec[blockIdx.x - 1]);

	// skip to the right input string
	if(blockIdx.x > 0)
		input += (cur_size_vec[blockIdx.x - 1]/fetch_bytes); 

	// get the right final_vector for reading the initial state and
	// storing the final output
	unsigned int st_vec_length             = st_vec_lengths            [blockIdx.y];
	unsigned int accum_state_vector_length = accum_state_vector_lengths[blockIdx.y]; 
	unsigned int accum_nfa_table_length    = accum_nfa_table_lengths   [blockIdx.y];
	unsigned int accum_offset_table_length = accum_offset_table_lengths[blockIdx.y];
	
	ST_BLOCK *final_vector = _svs + st_vec_length*blockIdx.x + accum_state_vector_length*nstreams;

	// shared_base points to the beginning of shared memory
	ST_BLOCK *status_vector = &shared_base[0];	
	ST_BLOCK *future_status_vector = shared_base + st_vec_length;

	//Copy the initial status vector from global to shared to set the input state
	for(unsigned int j = myId; j < st_vec_length; j += thread_count)
		//final_vector here is a misnomer as it is set with the initial state bit enabled
		status_vector[j] = final_vector[j];
	__syncthreads();

	unsigned int limit = cur_size; //printf("cur_size %d\n",cur_size);

	//Payload loop
	for(unsigned int p=0; p<limit; p+=fetch_bytes, input++){	
		symbol_fetch Input_ = *input;//fetch 4 bytes from the input string
		for (unsigned int byt = 0; byt < fetch_bytes; byt++) {
			unsigned int Input = Input_ & 0xFF;//extract 1 byte
			Input_  = Input_ >> 8;//Input_ right-shifted by 8 bits
			
			// input_transition_table contains the cumulative number of transitions for each input symbol
			unsigned int tr_base   = input_transition_tables[Input   + accum_offset_table_length];
			unsigned int tr_number = input_transition_tables[Input+1 + accum_offset_table_length] - tr_base;
			
			// Reset the future status vector
			// Persistent (self-loop'd) states are never reset once reached.
#pragma unroll 2
			for(unsigned w = myId; w < st_vec_length; w += thread_count)
				//future_status_vector[w] = persistents[w + accum_state_vector_length] & status_vector[w];
				future_status_vector[w] = 0;//might work too, since persistents vector is not used
			__syncthreads();

			for(unsigned int i=myId; i<tr_number; i+=thread_count) {
				// Each thread reads 1 transition at each step.
				st_t dst_state = nfa_tables[i + tr_base + accum_nfa_table_length];
				st_t src_state = src_tables[i + tr_base + accum_nfa_table_length];  
		
// These macros are there to extract the relevant fields.
// Bits and chunks are there to select the right bit in the state vectors.
#define src_bit  (1 << (src_state % bit_sizeof(ST_BLOCK)))
#define dst_bit  (1 << (dst_state % bit_sizeof(ST_BLOCK)))
#define src_chunk (src_state / bit_sizeof(ST_BLOCK))
#define dst_chunk (dst_state / bit_sizeof(ST_BLOCK))

				ST_BLOCK lo_block = src_bit & status_vector[src_chunk];
				if(lo_block) {
					if (dst_state < 0) {//Added for matching operation: check if the dst state is an accepting state
						dst_state = -dst_state;
						tmp_match_count = atomicAdd(&shr_match_count, 1);//printf("Inside kernel-low, offset: %d, state: %d, count %d\n",p, dst_state, shr_match_count);
						//match_offset[match_vec_size*blockIdx.x + shr_match_count-1 + blockIdx.y*match_vec_size*nstreams] = p + byt;
						//match_states[match_vec_size*blockIdx.x + shr_match_count-1 + blockIdx.y*match_vec_size*nstreams] = dst_state;
						tmp_match.off = p + byt;
						tmp_match.stat= dst_state;
						match_array[tmp_match_count + match_vec_size*(blockIdx.x + blockIdx.y*nstreams)] = tmp_match;
					}
					atomicOr(&future_status_vector[dst_chunk], dst_bit);    //unsigned int atomicOr(unsigned int* address, unsigned int val);
				}
			}
			// Swap status_vector and future_status_vector
			if(status_vector == shared_base){
				status_vector = shared_base + st_vec_length;
				future_status_vector = shared_base;
			} else {
				status_vector = shared_base;
				future_status_vector = shared_base + st_vec_length;
			}
			__syncthreads();
		}
	}

	//Copy the result vector from shared to device memory
#pragma unroll
	for(unsigned int j = myId; j < st_vec_length; j += thread_count) {
		final_vector[j] = status_vector[j];
	}	
	match_count[blockIdx.x + blockIdx.y*gridDim.x] = shr_match_count;
	
	__syncthreads();
}
