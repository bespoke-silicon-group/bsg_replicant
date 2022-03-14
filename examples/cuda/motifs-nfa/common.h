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

#ifndef COMMON_H_
#define COMMON_H_

#include <bsg_manycore_errno.h>
#include <bsg_manycore_cuda.h>
#include <cstdlib>
#include <time.h>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>

#define SV_GLOBAL 1

#define ST_BLOCK unsigned int

#define TRUE 1
#define FALSE 0

#define INPUT_OK 0
#define EOI 1 //End-Of-Input
#define INPUT_ERROR 2

#define NFA_OK 10
#define NFA_ERROR 11

#define MAX_PKT_SIZE 1500
#define MAX_PKT_BURST 1000000
#define MAX_STREAM 100
#define MAX_BURST_SIZE 100000000


typedef unsigned long symbol_t;
typedef unsigned short out_symbol_t;

typedef unsigned char symbol;//version 2 -- note: each symbol has 1 byte
//typedef unsigned short symbol_fetch;//version 2 -- 2-byte fetches
typedef unsigned int symbol_fetch;//version 2 -- 4-byte fetches
//typedef unsigned long long symbol_fetch;//version 2 -- 8-byte fetches
#define fetch_bytes 4              //version 2: -- note: change this constant to the respective symbol_fetch

typedef int st_t;//version 2 -- note: use negative numbers for accepting states, which limits the total number of states

typedef struct _match_type{//version 2 -- note: merge two memory transactions into one in matching operations
    unsigned int off;
    unsigned int stat;
} match_type;

#define bit_sizeof(a) (sizeof(a)*8)

typedef struct _nfa{
	int states_number;
	ST_BLOCK *table;
	size_t size;
} NFA;

typedef struct _status_vector{
	int states_number;
	ST_BLOCK *vector;
	size_t size;
	ST_BLOCK *device;
} ST_VECTOR;

#define ROUND_UP(how_much, data_type) ((how_much)/(sizeof(data_type)*8) + !!((how_much)%(sizeof(data_type)*8)))
#define BYTE_ROUND_UP(num, den) ((num)/(den) + !!((num)%(den)))


#endif /* COMMON_H_ */
