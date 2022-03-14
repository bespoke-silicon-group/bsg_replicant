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

#include "cuda_allocator.h"

#include <boost/foreach.hpp>

#ifdef DEBUG
#include <iostream>
#endif

using namespace std;

CudaAllocator::CudaAllocator() {
	// Nop
	return;
}

CudaAllocator::~CudaAllocator() {
#ifdef DEBUG
	cout << "Destroyed allocator: pos 1, size of host_:" << host_.size() <<endl;
#endif
	//BOOST_FOREACH(void *p, host_) {
	//	w_cudaFreeHost(p);
	//}
#ifdef DEBUG
	cout << "Destroyed allocator: pos 2, size of device_:" << device_.size() << endl;
#endif
	//BOOST_FOREACH(void *p, device_) {
	//	w_cudaFree(p);
	//}
#ifdef DEBUG
	cout << "Destroyed allocator: pos 3" << endl;
#endif
}

void CudaAllocator::dealloc_host(void *ptr) {
	for(vector<void *>::iterator i = host_.begin(); i != host_.end(); ++i)
		if(*i == ptr) {
			w_cudaFreeHost(ptr);
			ptr = 0;
			host_.erase(i);
			break;
		}
	return;
}

void CudaAllocator::dealloc_device(void *ptr) {
	for(vector<void *>::iterator i = device_.begin(); i != device_.end(); ++i)
		if(*i == ptr) {
			w_cudaFree(ptr);
			ptr = 0;
			device_.erase(i);
			break;
		}
	return;
}

