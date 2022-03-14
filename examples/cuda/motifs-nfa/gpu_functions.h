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

#ifndef DEVICE_FUNCTIONS_H_
#define DEVICE_FUNCTIONS_H_

#include <device_functions.h>

#include "common.h"

#define word_size	bit_sizeof(ST_BLOCK)

__global__ void nfa_kernel(	st_t *nfa_tables,				
							st_t *src_tables,
							unsigned int *input_transition_tables,
							symbol_fetch *input,
							unsigned long *cur_size_vec,
							ST_BLOCK *_svs,// ST_BLOCK is just a fancy name for a chunk of 32-bit unsigned data
							unsigned int *st_vec_lengths,
							ST_BLOCK *persistents,
							unsigned int *match_count, match_type *match_array, unsigned int match_vec_size,
							unsigned int *accum_nfa_table_lengths, unsigned int *accum_offset_table_lengths, unsigned int *accum_state_vector_lengths );
#endif /* DEVICE_FUNCTIONS_H_ */
