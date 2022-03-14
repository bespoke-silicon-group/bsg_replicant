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

#include <cstdlib>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "burst.h"
#include "common.h"
#include "cuda_allocator.h"
#include "host_functions.h"
#include "globals.h"
#include "gpu_functions.h"
#include "transition_graph.h"
#include "utils.h"
					
using namespace std;

/*--------------------------------------------------------------------------------------------------*/
#ifdef TEXTURE_MEM_USE //Texture memory: NFA STATE TABLE
texture<st_t, cudaTextureType1D, cudaReadModeElementType> tex_nfa_tables;
texture<st_t, cudaTextureType1D, cudaReadModeElementType> tex_src_tables;
texture<unsigned int, cudaTextureType1D, cudaReadModeElementType> tex_input_transition_tables;
__global__ void nfa_kernel_texture(	symbol_fetch *input,
									unsigned long *cur_size_vec,
									ST_BLOCK *_svs,// ST_BLOCK is just a fancy name for a chunk of 32-bit unsigned data
									unsigned int *st_vec_lengths,
									ST_BLOCK *persistents,
									unsigned int *match_count, match_type *match_array, unsigned int match_vec_size,
									unsigned int *accum_nfa_table_lengths, unsigned int *accum_offset_table_lengths, unsigned int *accum_state_vector_lengths);
#endif
/*--------------------------------------------------------------------------------------------------*/
void GPUMemInfo ()
{
   size_t free_byte ;
   size_t total_byte ;
   cudaError_t cuda_status = cudaMemGetInfo( &free_byte, &total_byte ) ;
   if ( cudaSuccess != cuda_status ){   
      printf("Error: cudaMemGetInfo fails, %s \n", cudaGetErrorString(cuda_status) );   
      exit(1);   
   }
   double free_db = (double)free_byte ;
   double total_db = (double)total_byte ;
   double used_db = total_db - free_db ;

   printf("GPU memory usage: used = %lf MB, free = %lf MB, total = %f MB\n", used_db/1024.0/1024.0, free_db/1024.0/1024.0, total_db/1024.0/1024.0);
}
/*--------------------------------------------------------------------------------------------------*/
vector<set<unsigned> > nfa_execute(std::vector<TransitionGraph *> tg, Burst &burst, unsigned int n_subsets, 
#ifdef DEBUG
                                   int *rulestartvec,
#endif
                                   double *t_alloc, double *t_kernel, double *t_collect, int *blocksize, unsigned int *trans_per_sym, int blksiz_tuning){	

   struct timeval c0, c1, c2, c3, c33, c4;
   long seconds, useconds;
   unsigned int *h_match_count, *d_match_count;
   match_type   *h_match_array, *d_match_array;
   
   ofstream fp_report;
   char filename[200], bufftmp[10];
   
   st_t *d_nfa_tables, *d_src_tables;
   unsigned int *d_offset_tables;
   symbol *d_input;
   unsigned long *d_cur_size_vec;
   ST_BLOCK *d_svs, *d_persistents, *d_accepts;
   size_t max_shmem=0;
   unsigned int *accum_nfa_table_lengths, *accum_offset_table_lengths, *accum_state_vector_lengths, *st_vec_lengths;//Note: arrays contain accumulated values   
   unsigned int *d_accum_nfa_table_lengths, *d_accum_offset_table_lengths, *d_accum_state_vector_lengths, *d_st_vec_lengths;

//#ifdef DEBUG
	//cout << "------------- Preparing to launch kernel ---------------" << endl;
	//cout << "Packets (Streams or Number of CUDA blocks in x-dimension): " << burst.get_sizes().size() << endl;
	
	//cout << "Accumulated number of symbol per packet (stream): ";
	//for (int i = 0; i < burst.get_sizes().size(); i++)
	//	cout << burst.get_sizes()[i] << " ";
	//cout << endl;
    
	//cout << "Threads per block: " << cfg.get_threads_per_block() << endl;

	for (unsigned int i = 0; i < n_subsets; ++i) {//Changed
		cout << "Graph (NFA) " << i+1 << endl;
		//cout << "   + Shared memory size: " << cfg.get_state_vector(i).get_size()*2 << endl;
	    cout << "   + State count: " << cfg.get_state_count(i) << endl;
		cout << "   + State vector length: " << cfg.get_state_vector(i).get_size()/sizeof(ST_BLOCK) << endl;
 	    cout << endl;
		if (max_shmem <= cfg.get_state_vector(i).get_size()*2) max_shmem = cfg.get_state_vector(i).get_size()*2;
	}	
//#endif
    
	gettimeofday(&c0, NULL);
	
	unsigned int tmp_avg_count = burst.get_sizes()[burst.get_sizes().size()-1]/burst.get_sizes().size()*15/n_subsets;//just for now, size of each match array for each packet

	cout << "tmp_avg_count: "   << tmp_avg_count
         << ", n_packets: "     << burst.get_sizes().size() 
         << ", n_subsets: "     << n_subsets
         << ", Maximum matches allowed: " << (tmp_avg_count*burst.get_sizes().size()*n_subsets) << endl;
	
	h_match_array         = (match_type*)malloc ((tmp_avg_count*burst.get_sizes().size()) * n_subsets * sizeof(match_type));//just for now
	h_match_count         = (unsigned int*)malloc ((              burst.get_sizes().size()) * n_subsets * sizeof(unsigned int));//just for now 
	
    accum_nfa_table_lengths    = (unsigned int*)malloc (n_subsets * sizeof(unsigned int));
	accum_offset_table_lengths = (unsigned int*)malloc (n_subsets * sizeof(unsigned int));
	accum_state_vector_lengths = (unsigned int*)malloc (n_subsets * sizeof(unsigned int));
	st_vec_lengths             = (unsigned int*)malloc (n_subsets * sizeof(unsigned int));
	
	cudaMalloc( (void **) &d_match_array,  (tmp_avg_count*burst.get_sizes().size()) * n_subsets * sizeof(match_type));//just for now
    cudaMalloc( (void **) &d_match_count,  (              burst.get_sizes().size()) * n_subsets * sizeof(unsigned int));//just for now

	cudaMalloc( (void **) &d_accum_nfa_table_lengths,    n_subsets * sizeof(unsigned int));
	cudaMalloc( (void **) &d_accum_offset_table_lengths, n_subsets * sizeof(unsigned int));
	cudaMalloc( (void **) &d_accum_state_vector_lengths, n_subsets * sizeof(unsigned int));
    cudaMalloc( (void **) &d_st_vec_lengths,             n_subsets * sizeof(unsigned int));
	
	size_t tmp_nfa_table_total_size=0, tmp_offset_table_total_size=0;//in bytes
	size_t tmp_curr_nfa_table_size=0, tmp_curr_offset_table_size=0;//in bytes
	size_t tmp_accum_prev_nfa_table_size=0, tmp_accum_prev_offset_table_size=0;//in bytes
	size_t tmp_state_vector_total_size=0, tmp_curr_state_vector_size=0, tmp_accum_prev_state_vector_size=0;//in bytes
	
	for (unsigned int i = 0; i < n_subsets; i++) {//Find total size (in bytes) of each data structure
		tmp_nfa_table_total_size    +=  tg[i]->get_nfa_table_size();
		tmp_offset_table_total_size +=  tg[i]->get_offset_table_size();
		tmp_state_vector_total_size +=  tg[i]->get_mutable_persistent_states().get_size();
		st_vec_lengths[i]            =  cfg.get_state_vector(i).get_size()/sizeof(ST_BLOCK);
	}
	cudaMalloc((void **) &d_nfa_tables,    tmp_nfa_table_total_size);//Allocate device memory
    cudaMalloc((void **) &d_src_tables,    tmp_nfa_table_total_size);
	cudaMalloc((void **) &d_offset_tables, tmp_offset_table_total_size);
	cudaMalloc((void **) &d_persistents,   tmp_state_vector_total_size);
	cudaMalloc((void **) &d_accepts,       tmp_state_vector_total_size);
	
	//GPUMemInfo();
	
	for (unsigned int i = 0; i < n_subsets; i++){//Copy to device memory
		cudaError_t retval1, retval2, retval3, retval4, retval5;
		tmp_curr_nfa_table_size    =  tg[i]->get_nfa_table_size();
		tmp_curr_offset_table_size =  tg[i]->get_offset_table_size();
		tmp_curr_state_vector_size =  tg[i]->get_mutable_persistent_states().get_size();
		
		if (i==0){
			retval1 = cudaMemcpy( d_nfa_tables,    tg[i]->get_nfa_table(),                                 tmp_curr_nfa_table_size,    cudaMemcpyHostToDevice);
			retval2 = cudaMemcpy( d_src_tables,    tg[i]->get_src_table(),                                 tmp_curr_nfa_table_size,    cudaMemcpyHostToDevice);
			retval3 = cudaMemcpy( d_offset_tables, tg[i]->get_offset_table(),                              tmp_curr_offset_table_size, cudaMemcpyHostToDevice);
			retval4 = cudaMemcpy( d_persistents,   tg[i]->get_mutable_persistent_states().get_host(false), tmp_curr_state_vector_size, cudaMemcpyHostToDevice);
			retval5 = cudaMemcpy( d_accepts,       tg[i]->get_accept_states().get_host(false),             tmp_curr_state_vector_size, cudaMemcpyHostToDevice);
		}
		else{
			tmp_accum_prev_nfa_table_size    +=  tg[i-1]->get_nfa_table_size();
			tmp_accum_prev_offset_table_size +=  tg[i-1]->get_offset_table_size();
			tmp_accum_prev_state_vector_size +=  tg[i-1]->get_mutable_persistent_states().get_size();
	
			retval1 = cudaMemcpy( &d_nfa_tables   [tmp_accum_prev_nfa_table_size/sizeof(st_t)],        tg[i]->get_nfa_table(),    tmp_curr_nfa_table_size,    cudaMemcpyHostToDevice);
			retval2 = cudaMemcpy( &d_src_tables   [tmp_accum_prev_nfa_table_size/sizeof(st_t)],        tg[i]->get_src_table(),    tmp_curr_nfa_table_size,    cudaMemcpyHostToDevice);
			retval3 = cudaMemcpy( &d_offset_tables[tmp_accum_prev_offset_table_size/sizeof(unsigned int)], tg[i]->get_offset_table(), tmp_curr_offset_table_size, cudaMemcpyHostToDevice);
			retval4 = cudaMemcpy( &d_persistents  [tmp_accum_prev_state_vector_size/sizeof(ST_BLOCK)],     tg[i]->get_mutable_persistent_states().get_host(false), tmp_curr_state_vector_size, cudaMemcpyHostToDevice);
			retval5 = cudaMemcpy( &d_accepts      [tmp_accum_prev_state_vector_size/sizeof(ST_BLOCK)],     tg[i]->get_accept_states().get_host(false),             tmp_curr_state_vector_size, cudaMemcpyHostToDevice);
		}
		accum_nfa_table_lengths[i]    = tmp_accum_prev_nfa_table_size/sizeof(st_t);
        accum_offset_table_lengths[i] = tmp_accum_prev_offset_table_size/sizeof(unsigned int);
		accum_state_vector_lengths[i] = tmp_accum_prev_state_vector_size/sizeof(ST_BLOCK);
	
		CUDA_CHECK(retval1, "Error while copying nfa table to device memory");
		CUDA_CHECK(retval2, "Error while copying src table to device memory");
		CUDA_CHECK(retval3, "Error while copying offset table to device memory");
		CUDA_CHECK(retval4, "Error while copying persistent state vector to device memory");
		CUDA_CHECK(retval5, "Error while copying accepting state vector to device memory");
	}

	d_input = burst.get_d_payloads();
	d_cur_size_vec = burst.get_d_sizes();
	d_svs = burst.get_mutable_state_vectors_device();//Changed
	
	cudaMemcpy( d_accum_nfa_table_lengths,    accum_nfa_table_lengths,    n_subsets * sizeof(unsigned int), cudaMemcpyHostToDevice);
	cudaMemcpy( d_accum_offset_table_lengths, accum_offset_table_lengths, n_subsets * sizeof(unsigned int), cudaMemcpyHostToDevice);
	cudaMemcpy( d_accum_state_vector_lengths, accum_state_vector_lengths, n_subsets * sizeof(unsigned int), cudaMemcpyHostToDevice);
	cudaMemcpy( d_st_vec_lengths,             st_vec_lengths,             n_subsets * sizeof(unsigned int), cudaMemcpyHostToDevice);
	
	GPUMemInfo();
	
	//Theoretical occupancy calculation
	//Finding maximum number of transitions per symbol (character)
	unsigned int max_trans_max=0;
	unsigned int min_trans_min=4000000000;
	unsigned int avg_trans=0;
	for (unsigned int i = 0; i < n_subsets * (256+1); i++) { 
		if (max_trans_max <= trans_per_sym[i]) max_trans_max = trans_per_sym[i];
		if (min_trans_min >= trans_per_sym[i]) min_trans_min = trans_per_sym[i];
		avg_trans += trans_per_sym[i];
	}
	avg_trans = avg_trans/(n_subsets * (256+1));
	
	for (unsigned int j = 0; j < n_subsets; j++){
		unsigned int tmp_max_trans=0;
		unsigned int tmp_min_trans=4000000000;
		unsigned int tmp_avg_trans=0;
		for (unsigned int i = 0; i < (256+1); i++){ 
			if (tmp_max_trans <= trans_per_sym[j*(256+1)+i]) tmp_max_trans = trans_per_sym[j*(256+1)+i];
			if (tmp_min_trans >= trans_per_sym[j*(256+1)+i]) tmp_min_trans = trans_per_sym[j*(256+1)+i];
			tmp_avg_trans += trans_per_sym[j*(256+1)+i];
		}
		tmp_avg_trans = tmp_avg_trans/(256+1);
		//printf("Subset %d: max_trans = %d, min_trans = %d, avg_trans = %d\n", j, tmp_max_trans, tmp_min_trans, tmp_avg_trans);
	}
	//
		
	int device;
	cudaDeviceProp props;
	cudaGetDevice(&device);
	cudaGetDeviceProperties(&props, device); printf("GPU MultiProcessors: %d\n", props.multiProcessorCount);
	
	int blockSize_init; // The launch configurator returned block size 
	int minGridSize;    // The minimum grid size needed to achieve the maximum occupancy for a full device launch 
	int maxActiveBlocks_init;
	cudaOccupancyMaxPotentialBlockSize( &minGridSize, &blockSize_init, nfa_kernel, max_shmem, 0);
	cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks_init, nfa_kernel, blockSize_init, max_shmem);
	float occupancy_init = (maxActiveBlocks_init * blockSize_init / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;
	//printf("Initial theoretical GPU launch info: minGridSize = %d, blockSize_init = %d, maxActiveBlocks_init = %d, occupancy_init: %f\n", minGridSize, blockSize_init, maxActiveBlocks_init, occupancy_init);
	
	//blockSize_init found by cudaOccupancyMaxPotentialBlockSize() sometimes is smaller than 1024 and sometimes can not give best performance as 1024 with small number of n_subsets and number of packets. So, we decided to overwrite with 1024 (hard coded, we know that 1024 can be accepted. An automatic code is needed in future) but still use the original occupancy_init.	
	if ( (burst.get_sizes().size()) * n_subsets <= props.multiProcessorCount)
		blockSize_init = 1024;
	
	int blockSize_ = blockSize_init;
	*blocksize  = blockSize_init;
	int maxActiveBlocks = maxActiveBlocks_init;
	float occupancy = occupancy_init;
	while (((maxActiveBlocks*props.multiProcessorCount < (burst.get_sizes().size()) * n_subsets) || (abs(occupancy - occupancy_init) > 5)) && 
	       (blockSize_>32)) {
		blockSize_-=32;
		cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, nfa_kernel, blockSize_, max_shmem);
		occupancy = (maxActiveBlocks * blockSize_ / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;
		//printf("Inter. theoretical GPU launch info: blockSize_ = %d, maxActiveBlocks = %d, occupancy: %f\n", blockSize_, maxActiveBlocks, occupancy);
		if (abs(occupancy - occupancy_init) <= 5){	
			*blocksize = blockSize_;
		}
	}
		//Calculate blocksize based on max_trans_max
		blockSize_ = blockSize_init;
		int blocksize_trans  = blockSize_init;
		maxActiveBlocks = maxActiveBlocks_init;
		occupancy = occupancy_init;
		while ((blockSize_ > max_trans_max) && (blockSize_>32)) {
			blockSize_-=32;
			cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, nfa_kernel, blockSize_, max_shmem);
			occupancy = (maxActiveBlocks * blockSize_ / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;
			//printf("Inter. theoretical GPU launch info (use max_trans_max): blockSize_ = %d, maxActiveBlocks = %d, occupancy: %f\n", blockSize_, maxActiveBlocks, occupancy);
			if ((abs(occupancy - occupancy_init) <= 5) && (blockSize_ > max_trans_max)){	
				blocksize_trans = blockSize_;
			}
		}
		cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, nfa_kernel, blocksize_trans, max_shmem);
		occupancy = (maxActiveBlocks * blocksize_trans / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;
		//printf("Final theoretical GPU launch info: blocksize_trans = %d, maxActiveBlocks = %d, occupancy: %f, max_trans_max = %d, min_trans_min = %d, avg_trans = %d\n", blocksize_trans, maxActiveBlocks, occupancy, max_trans_max, min_trans_min, avg_trans);
		//End: Calculate blocksize based on max_trans_max
	
	cudaOccupancyMaxActiveBlocksPerMultiprocessor( &maxActiveBlocks, nfa_kernel, *blocksize, max_shmem);
	occupancy = (maxActiveBlocks * (*blocksize) / props.warpSize) / (float)(props.maxThreadsPerMultiProcessor / props.warpSize)*100;
		
	//printf("Final theoretical GPU launch info: blocksize = %d, maxActiveBlocks = %d, occupancy: %f, max_trans_max = %d, min_trans_min = %d, avg_trans = %d\n", *blocksize, maxActiveBlocks, occupancy, max_trans_max, min_trans_min, avg_trans);
	
	if (*blocksize >= blocksize_trans) *blocksize = blocksize_trans;//find min		
	
	// Bind textures to d_nfa_tables, d_src_tables, d_offset_tables
#ifdef TEXTURE_MEM_USE
		cudaChannelFormatDesc channelDesc1 = cudaCreateChannelDesc<st_t>();
		cudaChannelFormatDesc channelDesc2 = cudaCreateChannelDesc<unsigned int>();
		cudaBindTexture(0, tex_nfa_tables, d_nfa_tables, channelDesc1, tmp_nfa_table_total_size);
		cudaBindTexture(0, tex_src_tables, d_src_tables, channelDesc1, tmp_nfa_table_total_size);
		cudaBindTexture(0, tex_input_transition_tables, d_offset_tables, channelDesc2, tmp_offset_table_total_size);	
		printf("Texture memory usage: %.0lf bytes (%lf Mbytes)\n", (2*tmp_nfa_table_total_size + tmp_offset_table_total_size)/1.0, (2*tmp_nfa_table_total_size + tmp_offset_table_total_size)/1024.0/1024.0);
#endif
	
	gettimeofday(&c1, NULL);
	
	// Launch kernel (asynchronously)
	//printf("Size of symbol = %d, Size of unsigned char = %d\n",sizeof(symbol), sizeof(unsigned char));
	dim3 block(cfg.get_threads_per_block(),1);
	dim3 grid(burst.get_sizes().size(),n_subsets);
    
	if (blksiz_tuning == 1) {
		block.x = *blocksize;
		printf("Blocksize tuning is used!\n");
	}
	else {
		printf("Blocksize tuning is NOT used!\n");
	}	
	cout << "GPU launch info: block.x = " << block.x << ", grid.x = " << grid.x << ", grid.y = " << grid.y << ", shmem = " << max_shmem << endl;
    cout << "GPU kernel running ... " << endl;
#ifdef TEXTURE_MEM_USE
		printf("NFA STATE TABLE stored in texture memory!\n");
		nfa_kernel_texture<<<grid, block, max_shmem>>>(/* TODO: the next three are w/o reference! */
												(symbol_fetch*)d_input,
												d_cur_size_vec,
												d_svs,
												d_st_vec_lengths,
												d_persistents,
												d_match_count, d_match_array, tmp_avg_count,
												d_accum_nfa_table_lengths, d_accum_offset_table_lengths, d_accum_state_vector_lengths);
#else
		printf("NFA STATE TABLE stored in global memory!\n");
		nfa_kernel<<<grid, block, max_shmem>>>(/* TODO: the next three are w/o reference! */
												d_nfa_tables,
												d_src_tables,
												d_offset_tables,
												(symbol_fetch*)d_input,
												d_cur_size_vec,
												d_svs,
												d_st_vec_lengths,
												d_persistents,
												d_match_count, d_match_array, tmp_avg_count,
												d_accum_nfa_table_lengths, d_accum_offset_table_lengths, d_accum_state_vector_lengths);	
#endif
	
	cudaThreadSynchronize();
	
	gettimeofday(&c2, NULL);
	
    cout << "GPU kernel done!!!" << endl;

#ifdef TEXTURE_MEM_USE		
	// unbind textures from d_nfa_tables, d_src_tables, d_offset_tables
    cudaUnbindTexture(tex_nfa_tables);
	cudaUnbindTexture(tex_src_tables);
	cudaUnbindTexture(tex_input_transition_tables);	
#endif
	
	//seconds  = c2.tv_sec  - c1.tv_sec;
	//useconds = c2.tv_usec - c1.tv_usec;
    //*t_kernel= ((double)seconds * 1000 + (double)useconds/1000.0);
	//printf("host_functions.cu: t_kernel= %lf(ms)\n", *t_kernel);
	
	cudaMemcpy( h_match_count,  d_match_count,                 burst.get_sizes().size()  * n_subsets * sizeof(unsigned int), cudaMemcpyDeviceToHost);
	cudaMemcpy( h_match_array, d_match_array, (tmp_avg_count*burst.get_sizes().size()) * n_subsets * sizeof(match_type), cudaMemcpyDeviceToHost);

    gettimeofday(&c3, NULL);		

	// Collect results
	//Temporarily comment the following FOR loop
    printf("Collecting results and saving into files ...\n");
	unsigned int total_matches=0;
	for (unsigned int i = 0; i < n_subsets; i++) {
#ifdef TEXTURE_MEM_USE
		strcpy (filename,"Report_tex_");
#else
        strcpy (filename,"Report_global_");
#endif	
		snprintf(bufftmp, sizeof(bufftmp),"%d",n_subsets);
		strcat (filename,bufftmp);
		strcat (filename,"_");
		snprintf(bufftmp, sizeof(bufftmp),"%d",i+1);
		strcat (filename,bufftmp);
		strcat (filename,".txt");
		fp_report.open (filename); //cout << "Report filename:" << filename << endl;
		tg[i]->mapping_states2rules(&h_match_count[burst.get_sizes().size()*i], &h_match_array[tmp_avg_count*burst.get_sizes().size()*i], 
		                            tmp_avg_count, burst.get_sizes(), burst.get_padded_sizes(), fp_report
#ifdef DEBUG
									, rulestartvec, i 
#endif
									                );
		fp_report.close();
		for (unsigned int j = 0; j < burst.get_sizes().size(); j++)
			total_matches += h_match_count[j + burst.get_sizes().size()*i];
	}
	printf("Host - Total number of matches %d\n", total_matches);

    gettimeofday(&c33, NULL);

	vector<set<unsigned> >batch_accepted_rules;
    
	// Free some memory
	//d__sv = burst.get_mutable_state_vectors_device(), d_input = burst.get_d_payloads(), and  d_cur_size_vec = burst.get_d_sizes() are freed outside the host_functions.cu
	//tg->get_mutable_persistent_states().free_device();//free device mem for d_persistent = tg->get_mutable_persistent_states().get_device();
	
	cudaFree(d_match_count);
	cudaFree(d_match_array);
	cudaFree(d_accum_nfa_table_lengths);
	cudaFree(d_accum_offset_table_lengths);
	cudaFree(d_accum_state_vector_lengths);
	cudaFree(d_st_vec_lengths);
	cudaFree(d_nfa_tables);
    cudaFree(d_src_tables);
	cudaFree(d_offset_tables);
	cudaFree(d_persistents);
	cudaFree(d_accepts);
	
	free(h_match_count);
	free(h_match_array);
	free(accum_nfa_table_lengths);
	free(accum_offset_table_lengths);
	free(accum_state_vector_lengths);
	free(st_vec_lengths);
	
	gettimeofday(&c4, NULL);
	
	seconds  = c1.tv_sec  - c0.tv_sec;
	useconds = c1.tv_usec - c0.tv_usec;
    *t_alloc = ((double)seconds * 1000 + (double)useconds/1000.0);
	
	seconds  = c2.tv_sec  - c1.tv_sec;
	useconds = c2.tv_usec - c1.tv_usec;
    *t_kernel= ((double)seconds * 1000 + (double)useconds/1000.0);
	
	seconds  = c3.tv_sec  - c2.tv_sec;
	useconds = c3.tv_usec - c2.tv_usec;
    *t_collect = ((double)seconds * 1000 + (double)useconds/1000.0);

	seconds  = c33.tv_sec  - c3.tv_sec;
	useconds = c33.tv_usec - c3.tv_usec;
	printf("host_functions.cu: t_postprocesscpu= %lf(ms)\n", ((double)seconds * 1000 + (double)useconds/1000.0));

	seconds  = c4.tv_sec  - c33.tv_sec;
	useconds = c4.tv_usec - c33.tv_usec;
	printf("host_functions.cu: t_free= %lf(ms)\n", ((double)seconds * 1000 + (double)useconds/1000.0));

	return batch_accepted_rules;
}
