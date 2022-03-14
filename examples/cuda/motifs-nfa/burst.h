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

#ifndef BURST_
#define BURST_

#include <vector>

#include "common.h"
#include "cuda_allocator.h"
#include "half_trie.h"

class StateVector;

class Burst {
	private:
		std::vector<symbol>symbols_;
		std::vector<unsigned char> payloads_;
		std::vector<unsigned long> sizes_;
		
		//-- note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
		std::vector<unsigned long> padded_sizes_;//store the numbers of padded bytes of each packet 
		
		short required_translations_;
		
		StateVector *sv_tmp;
		std::vector<StateVector *> sv_;
		mutable ST_BLOCK *d_sv_;
		
		CudaAllocator allocator_;
		symbol *d_payloads_;
		unsigned long *d_sizes_;
	
		unsigned int translate_payload(const std::vector<unsigned char> &payload,
				const std::vector<half_trie::HalfTrie<symbol_t, out_symbol_t> *> &trns);
				
	public:
		Burst();
		~Burst();
		void append_payload(const std::vector<unsigned char> payload);
		symbol *get_d_payloads(hb_mc_device_t *device);
		unsigned long *get_d_sizes(void);

		ST_BLOCK *get_mutable_state_vectors_device(hb_mc_device_t *device);

		void init_state_vector(const StateVector &initial_sv, unsigned int gid);
		
		const std::vector<unsigned long> &get_sizes(hb_mc_device_t *device);
		const std::vector<unsigned char> &get_payloads(void);

		//-- note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
		void save_n_padded_bytes(unsigned long nbytes);
		const std::vector<unsigned long> &get_padded_sizes(void);

		void set_required_translations(short num_trans);
		
		void free_device();
};

#endif
