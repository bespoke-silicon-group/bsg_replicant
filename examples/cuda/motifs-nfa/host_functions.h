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

#ifndef HOST_FUNCTIONS_H_
#define HOST_FUNCTIONS_H_

#include <fstream>
#include <set>
#include <vector>

#include <stdio.h>

#include "common.h"
//#include "half_trie.h"
//#include "sparse.h"
#include "transition_graph.h"

class Burst;

std::vector<std::set<unsigned> > nfa_execute(std::vector<TransitionGraph *> tg, Burst &burst, unsigned int n_subsets, 
#ifdef DEBUG
                                             int *rulestartvec,
#endif											 
                                             double *t_alloc, double *t_kernel, double *t_collect, int *blocksize, unsigned int *trans_per_sym, int blksiz_tuning);


#endif /* HOST_FUNCTIONS_H_ */
