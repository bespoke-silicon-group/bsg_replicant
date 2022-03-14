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

#include "config_options.h"
#include "half_trie.h"

using namespace std;
using namespace half_trie;

ConfigOptions::ConfigOptions() {
	good_tries_ = 0;
	parallel_packets_ = 1;
	total_packets_ = -1;
	threads_per_block_ = 32;
	blocks_y_ = 1;
	trace_file_name_ = NULL;
	burst_size_ = parallel_packets_ * 2048;
#ifdef VALIDATE
	validate_filename_ = NULL;
#endif
}

unsigned int ConfigOptions::get_alphabet_size() const {
	return max_out_ + 1;
}

unsigned int ConfigOptions::get_burst_size() const {
	return burst_size_;
}

unsigned int ConfigOptions::get_good_tries() const {
	return good_tries_;
}

symbol_t ConfigOptions::get_max_out() const {
	return max_out_;
}

out_symbol_t *ConfigOptions::get_mutable_max_outs() {
	return max_outs_;
}

unsigned int ConfigOptions::get_max_tries() const {
	return MAX_TRIES;
}

unsigned int ConfigOptions::get_parallel_packets() const {
	return parallel_packets_;
}

unsigned int ConfigOptions::get_state_count(unsigned int gid) const {
	return state_count_[gid];
}

unsigned int ConfigOptions::get_threads_per_block() const {
	return threads_per_block_;
}

unsigned int ConfigOptions::get_blocks_y() const {
	return blocks_y_;
}

const char *ConfigOptions::get_trace_file_name() const {
	return trace_file_name_;
}

const StateVector &ConfigOptions::get_state_vector(unsigned int gid) const {
	return *state_vector_[gid];
}

unsigned int ConfigOptions::get_total_packets() const {
	return total_packets_;
}

vector<HalfTrie<symbol_t, out_symbol_t> *> &ConfigOptions::get_mutable_trns() {
	return trns_;
}

void ConfigOptions::set_burst_size(unsigned int burst_size) {
	burst_size_ = burst_size;
}

void ConfigOptions::set_good_tries(unsigned int gt) {
	good_tries_ = gt;
}

void ConfigOptions::set_max_out(symbol_t mo) {
	max_out_ = mo;
}

void ConfigOptions::set_max_outs(out_symbol_t *mo) {
	max_outs_ = mo;
}

void ConfigOptions::set_parallel_packets(unsigned int parallel_packets) {
	parallel_packets_ = parallel_packets;
	burst_size_ = parallel_packets * 2048;
}

void ConfigOptions::set_state_count(unsigned int state_count) {
	state_count_.push_back(state_count);
}

void ConfigOptions::set_state_vector(StateVector *state_vector) {
	state_vector_.push_back(state_vector);
}

void ConfigOptions::set_threads_per_block(unsigned int threads_per_block) {
	threads_per_block_ = threads_per_block;
}

void ConfigOptions::set_blocks_y(unsigned int blocks_y) {
	blocks_y_ = blocks_y;
}

void ConfigOptions::set_total_packets(unsigned int total_packets) {
	total_packets_ = total_packets;
}

void ConfigOptions::set_trace_file_name(char *trace_file_name) {
	trace_file_name_ = trace_file_name;
}

void ConfigOptions::add_trns(HalfTrie<symbol_t, out_symbol_t> *trns) {
	trns_.push_back(trns);
}

CudaAllocator &ConfigOptions::get_allocator() {
	return all_;
}
