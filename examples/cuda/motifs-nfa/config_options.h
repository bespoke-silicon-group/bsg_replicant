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

#ifndef CONFIG_OPTIONS_H_
#define CONFIG_OPTIONS_H_

#include "common.h"
#include "cuda_allocator.h"
#include "half_trie.h"

#include <stdio.h>

#include <fstream>
#include <vector>

class StateVector;

class ConfigOptions {
	private:
		enum {
			MAX_TRIES = 4,
		};

        std::vector<StateVector *> state_vector_;

		unsigned int burst_size_;
		unsigned int parallel_packets_;
		unsigned int pkt_burst_size_;
		
		std::vector<unsigned int> state_count_;
		
		unsigned int threads_per_block_;
		unsigned int blocks_y_;
		int total_packets_;
		char *trace_file_name_;

		symbol_t max_out_;
		out_symbol_t *max_outs_;
		std::vector<half_trie::HalfTrie<symbol_t, out_symbol_t> *> trns_;

		unsigned int good_tries_;

		CudaAllocator all_;

	public:
		ConfigOptions();

		unsigned int get_alphabet_size() const;
		unsigned int get_burst_size() const;
		unsigned int get_good_tries() const;
		symbol_t get_max_out() const;
		out_symbol_t *get_mutable_max_outs();
		unsigned int get_max_tries() const;
		const StateVector &get_state_vector(unsigned int gid) const;
		StateVector &get_mutable_initial_sv();
		std::vector<half_trie::HalfTrie<symbol_t, out_symbol_t> *>
			&get_mutable_trns();
		unsigned int get_parallel_packets() const;
		unsigned int get_state_count(unsigned int gid) const;
		unsigned int get_threads_per_block() const;
		unsigned int get_blocks_y() const;
		const char *get_trace_file_name() const;
		unsigned int get_total_packets() const;
		CudaAllocator &get_allocator();

		void set_burst_size(unsigned int burst_size);
		void set_good_tries(unsigned int good_tries);
		void set_max_out(symbol_t max_out);
		void set_max_outs(out_symbol_t *max_out);
		void set_parallel_packets(unsigned int parallel_packets);
		void set_state_count(unsigned int state_count);
		void set_state_vector(StateVector *state_vector);
		void set_threads_per_block(unsigned int threads_per_block);
		void set_blocks_y(unsigned int blocks_x);
		void set_total_packets(unsigned int total_packets);
		void set_trace_file_name(char * trace_filename);

		void add_trns(half_trie::HalfTrie<symbol_t, out_symbol_t> *trns);
};

#endif /* HOST_FUNCTIONS_H_ */
