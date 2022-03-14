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

#include "burst.h"
#include "globals.h"
#include "state_vector.h"

#include <vector>

using namespace std;

Burst::Burst() {
	required_translations_ = -1;
	d_payloads_ = NULL;
	d_sizes_ = NULL;
}

Burst::~Burst() {
	for (unsigned int i = 0; i < sv_.size(); i++) {//Changed
		delete sv_[i];
	}
}

void Burst::set_required_translations(short num_trans) {
	required_translations_ = num_trans;
}

ST_BLOCK *Burst::get_mutable_state_vectors_device(hb_mc_device_t *device) {//Changed
		
	size_t tmp_total_size=0;//in bytes
	size_t tmp_curr_size=0;//in bytes
	size_t tmp_accum_prev_size=0;//in bytes
	
	for (unsigned int i = 0; i < sv_.size(); i++) tmp_total_size +=  sv_[i]->get_size();
	
	BSG_CUDA_CALL(hb_mc_device_malloc(device, tmp_total_size, &d_sv_));
    
	for (unsigned int i = 0; i < sv_.size(); i++){
		tmp_curr_size =  sv_[i]->get_size();
		if (i==0)
			BSG_CUDA_CALL(hb_mc_device_memcpy (device, d_sv_, sv_[i]->get_host(), tmp_curr_size, HB_MC_MEMCPY_TO_DEVICE));
		else{
			tmp_accum_prev_size +=  sv_[i-1]->get_size();
			BSG_CUDA_CALL(hb_mc_device_memcpy (device, &d_sv_[tmp_accum_prev_size/sizeof(ST_BLOCK)], sv_[i]->get_host(), tmp_curr_size, HB_MC_MEMCPY_TO_DEVICE));
		}
	}
	return d_sv_;	
}

unsigned long *Burst::get_d_sizes(hb_mc_device_t *device) {
	if (d_sizes_ == NULL) {
		d_sizes_ =
			allocator_.alloc_device<unsigned long>(device, sizes_.size() * 
					sizeof(*d_sizes_));
	}
	BSG_CUDA_CALL(hb_mc_device_memcpy (device, d_sizes_, &sizes_[0], sizes_.size() * sizeof(*d_sizes_),
			HB_MC_MEMCPY_TO_DEVICE));
	return d_sizes_;
}

const vector<unsigned long> &Burst::get_sizes() {
	return sizes_;
}

const vector<unsigned char> &Burst::get_payloads(void) {
	return payloads_;
}

symbol *Burst::get_d_payloads(hb_mc_device_t *device) {
	if (d_payloads_ == NULL) {
		d_payloads_ =
			allocator_.alloc_device<symbol>(device, symbols_.size() * sizeof(*d_payloads_));
	}
	BSG_CUDA_CALL(hb_mc_device_memcpy (device, d_payloads_, &symbols_[0],
			symbols_.size() * sizeof(*d_payloads_), HB_MC_MEMCPY_TO_DEVICE);
	return d_payloads_;
}

void Burst::init_state_vector(const StateVector &initial_sv, unsigned int gid) {//Changed
	//if (!sv_[gid]) {
		sv_tmp = new StateVector (cfg.get_state_vector(gid).get_size() * bit_sizeof(char) * sizes_.size(), allocator_);
		sv_.push_back(sv_tmp);
	//}

	for(unsigned i = 0; i < sizes_.size(); ++i)
		sv_[gid]->set_bits(initial_sv,	i * initial_sv.get_size() * bit_sizeof(char));
}

//-- note: padding to each packet if packet_size is not evenly divided by fetch_bytes (e.g. 4, 8)
const vector<unsigned long> &Burst::get_padded_sizes() {
	return padded_sizes_;
}
void Burst::save_n_padded_bytes(unsigned long nbytes){
	padded_sizes_.push_back(nbytes + (padded_sizes_.empty() ? 0 : padded_sizes_.back()));//Note: Accumulating the numbers of padded bytes of each packet
}

void Burst::append_payload(const std::vector<unsigned char> payload) {
	unsigned int translated_size = 0;

	if (required_translations_ < 0) {
		payloads_.insert(payloads_.end(),	payload.begin(), payload.end());
		translated_size = payload.size();
	} else {	
		translated_size = translate_payload(payload, cfg.get_mutable_trns());//Note: the runtime enters this
	}
	sizes_.push_back(translated_size + (sizes_.empty() ? 0 : sizes_.back()));//Note: Accumulating payload.size() of each packet
}

unsigned int Burst::translate_payload (const vector<unsigned char> &payload,
		const vector<half_trie::HalfTrie<symbol_t, out_symbol_t> *> &trns) {
	//if (required_translations_ == 0) {//version 2
		//Note: the runtime enters this
		for(unsigned i = 0; i<payload.size(); ++i) {
			symbols_.push_back(payload[i]);
		}
		return payload.size();
}

void Burst::free_device(){
	allocator_.dealloc_device(d_payloads_);
	allocator_.dealloc_device(d_sizes_);
}
