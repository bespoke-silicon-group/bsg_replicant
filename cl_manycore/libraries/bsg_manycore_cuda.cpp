#ifndef COSIM
#include <bsg_manycore_cuda.h>  
#include <bsg_manycore_driver.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_memory_manager.h>
#include <bsg_manycore_elf.h>
#include <bsg_manycore_mem.h>
#include <bsg_manycore_loader.h>
#else
#include "bsg_manycore_cuda.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore_tile.h"
#include "bsg_manycore_memory_manager.h"
#include "bsg_manycore_elf.h"
#include "bsg_manycore_mem.h"
#include "bsg_manycore_loader.h"
#endif

static const uint32_t KERNEL_REG = 0x1000 >> 2; //!< EPA of kernel. 
static const uint32_t ARGC_REG = 0x1004 >> 2; //!< EPA of number of arguments kernel will use. 
static const uint32_t ARGV_REG = 0x1008 >> 2; //!< EPA of arguments for kernel. 
static const uint32_t SIGNAL_REG = 0x100c >> 2; //!< EPA of register that holds signal address. Tile will write to this address once it completes the kernel.   


static const uint32_t FINISH_ADDRESS = 0xC0DA; //!< EPA to which tile group sends a finish packet once it finishes executing a kernel  

static awsbwhal::MemoryManager *mem_manager[1] = {(awsbwhal::MemoryManager *) 0}; /* This array has an element for every EVA <-> NPA mapping. Currently, only one mapping is supported. */

static uint32_t const DRAM_SIZE = 0x80000000;

/*!
 * Gets the x coordinates of a list of tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_x(tile_t *tiles, uint8_t *x_list, uint32_t num_tiles) {
	if (!tiles || !x_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		x_list[i] = tiles[i].x;
	}
	return HB_MC_SUCCESS;
}

/*!
 * Gets the x coordinates of a list of tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_y(tile_t *tiles, uint8_t *y_list, uint32_t num_tiles) {
	if (!tiles || !y_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		y_list[i] = tiles[i].y;
	}
	return HB_MC_SUCCESS;
}

/*!
 * creates a awsbwhal::MemoryManager object and stores it in a global table.
 * @param eva_id which specifies which EVA <-> NPA mapping.
 * @param elf path to ELF binary
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
static int hb_mc_create_memory_manager (eva_id_t eva_id, char *elf) {
	eva_t program_end;
	if (symbol_to_eva(elf, "_bsg_dram_end_addr", &program_end) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	uint32_t alignment = 32;
	uint32_t start = program_end + alignment - (program_end % alignment); /* start at the next aligned block */
	uint32_t size = DRAM_SIZE;
	mem_manager[eva_id] = new awsbwhal::MemoryManager(DRAM_SIZE, start, alignment); 
	return HB_MC_SUCCESS;	
}

static int hb_mc_write_tile_reg(uint8_t fd, eva_t eva_id, tile_t *tile, uint32_t epa, uint32_t val) {
	int error =  hb_mc_copy_to_epa (fd, tile->x, tile->y, epa, &val, 1);
	if (error != HB_MC_SUCCESS)
		return HB_MC_FAIL; /* could not memcpy */
	return HB_MC_SUCCESS;	
}

/*
 * Initializes Manycore tiles so that they may run kernels.
 * @param fd userspace file descriptor, which must be obtained from hb_mc_host_init.
 * @param eva_id specifies what the EVA-NPA mapping is.
 * @param tiles an array of tile_t structs to initialize.
 * @param num_tiles the number of tiles to initialize.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_init (uint8_t *fd, eva_id_t eva_id, char *elf, tile_t *tiles, uint32_t num_tiles) {
	
	int error = hb_mc_fifo_init(fd); 
	if (error != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_device_init() --> hb_mc_host_init(): failed to initialize host.\n");
		return HB_MC_FAIL;
	}
	
	if (eva_id != 0) {
		return HB_MC_FAIL; /* eva_id not supported */
	} 
	
	for (int i = 0; i < num_tiles; i++) { /* initialize tiles */
		hb_mc_tile_freeze(*fd, tiles[i].x, tiles[i].y);
		hb_mc_tile_set_group_origin(*fd, tiles[i].x, tiles[i].y, tiles[i].origin_x, tiles[i].origin_y);
	}


	/* load the elf into each tile */
	uint8_t x_list[num_tiles], y_list[num_tiles];	
	hb_mc_get_x(tiles, &x_list[0], num_tiles);
	hb_mc_get_y(tiles, &y_list[0], num_tiles); 
	hb_mc_load_binary(*fd, elf, &x_list[0], &y_list[0], num_tiles);
	/* create a memory manager object */
	if (hb_mc_create_memory_manager(eva_id, elf) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
  	
	/* unfreeze the tile group */
	for (int i = 0; i < num_tiles; i++) {
		error = hb_mc_write_tile_reg(*fd, eva_id, &tiles[i], KERNEL_REG, 0x1); /* initialize the kernel register */
		if (error != HB_MC_SUCCESS)
			return HB_MC_FAIL;
		hb_mc_tile_unfreeze(*fd, tiles[i].x, tiles[i].y);
	}
	return HB_MC_SUCCESS;
}

/*!
 * Initializes Manycore tiles so that they may run kernels.
 * @param fd userspace file descriptor
 * @param eva_id specifies what the EVA-NPA mapping is.
 * @param tiles an array of tile_t structs to initialize.
 * @param num_tiles the number of tiles to initialize.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_finish (uint8_t fd, eva_id_t eva_id, tile_t *tiles, uint32_t num_tiles) {

	if (eva_id != 0) {
		return HB_MC_FAIL; /* eva_id not supported */
	} 

	else if (!mem_manager[eva_id])
		return HB_MC_SUCCESS; /* there is no memory manager to deinitialize */
	
	delete(mem_manager[eva_id]);
	
	for (int i = 0; i < num_tiles; i++) { /* freeze tiles */
		hb_mc_tile_freeze(fd, tiles[i].x, tiles[i].y);
	}

	int error = hb_mc_fifo_finish(fd);
	if (error != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_device_finish() --> hb_mc_host_finish(): failed to terminate host.\n");
		return HB_MC_FAIL;
	}

	return HB_MC_SUCCESS;
}

/*!
 * This function is for testing hb_mc_device_init() only. 
 */
void _hb_mc_get_mem_manager_info(eva_id_t eva_id, uint32_t *start, uint32_t *size) {
	if (!mem_manager[eva_id]) {
		printf("_hb_mc_get_mem_manager_info(): mem manager not initialized.\n");
		return;
	}
	*start = mem_manager[eva_id]->start();
	*size =mem_manager[eva_id]->size();
}

/*!
 * allocates memory in Manycore
 *@param eva_id specifies EVA-NPA mapping.
 *@param size in bytes.
 *@param eva returned EVA address. Set to 0 on failure.
 *@return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. This function can fail if eva_id is invalid or of the memory manager corresponding to eva_id has not been initialized.
 */
int hb_mc_device_malloc (eva_id_t eva_id, uint32_t size, /*out*/ eva_t *eva) {
        *eva = 0;
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid EVA ID */
	}
	else if (!mem_manager[eva_id]) {
		return HB_MC_FAIL; /* memory manager has not been initialized */
	}

	eva_t result = mem_manager[eva_id]->alloc(size);
	if (result == awsbwhal::MemoryManager::mNull)
		return HB_MC_FAIL; /* could not allocate */
        *eva = result;
	return HB_MC_SUCCESS;
}

/*!
 * frees Hammerblade Manycore memory.
 *@param eva_id specifies EVA-NPA mapping.
 *@param eva address to free.
 *@return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. This function can fail if eva_id is invalid or of the memory manager corresponding to eva_id has not been initialized.
 */
int hb_mc_device_free (eva_id_t eva_id, eva_t eva) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid EVA ID */
	}
	else if (!mem_manager[eva_id]) {
		return HB_MC_FAIL; /* memory manager has not been initialized */
	}

	mem_manager[eva_id]->free(eva);
	return HB_MC_SUCCESS;
}

/*
 * caller must ensure eva_id is valid. */
static int hb_mc_cpy_to_eva (uint8_t fd, eva_id_t eva_id, eva_t dst, uint32_t *src) {
	npa_t npa;	
	int error = hb_mc_eva_to_npa(eva_id, dst, &npa);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not convert EVA to an NPA */
	}
	error = hb_mc_copy_to_epa (fd, npa.x, npa.y, npa.epa, src, 1 /* 1 word */);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not send data to Manycore */
	}
	return HB_MC_SUCCESS;
}

/*
 * caller must esure eva_id is valid. 
 * */
static int hb_mc_cpy_from_eva (uint8_t fd, eva_id_t eva_id, hb_mc_response_packet_t *dest, eva_t src) {
	npa_t npa;	
	int error = hb_mc_eva_to_npa(eva_id, src, &npa);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not convert EVA to an NPA */
	}
	error = hb_mc_copy_from_epa (fd, dest, npa.x, npa.y, npa.epa, 1 /* 1 word */);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not send data to Manycore */
	}
	return HB_MC_SUCCESS;
}

int hb_mc_device_memcpy (uint8_t fd, eva_id_t eva_id, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind) {
	if (eva_id != 0) 
		return HB_MC_FAIL; /* invalid EVA ID */

	else if (kind == hb_mc_memcpy_to_device) { /* copy to Manycore */
		eva_t dst_eva = (eva_t) reinterpret_cast<uintptr_t>(dst);
		for (int i = 0; i < count; i += sizeof(uint32_t)) { /* copy one word at a time */
			char *src_word = (char *) src + i;
			int error = hb_mc_cpy_to_eva(fd, eva_id, dst_eva + i, (uint32_t *) (src_word)); 		
			if (error != HB_MC_SUCCESS)
				return HB_MC_FAIL; /* copy failed */
		}
		return HB_MC_SUCCESS;	
	}
	
	else if (kind == hb_mc_memcpy_to_host) { /* copy to Host */
		eva_t src_eva = (eva_t) reinterpret_cast<uintptr_t>(src);
		for (int i = 0; i < count; i += sizeof(uint32_t)) { /* copy one word at a time */
                        // read in a packet
                        hb_mc_response_packet_t dst_packet;
			int error = hb_mc_cpy_from_eva(fd, eva_id, &dst_packet, src_eva + i);
			if (error != HB_MC_SUCCESS)
				return HB_MC_FAIL; /* copy failed */

                        // copy the word into caller dst buffer
                        uint32_t *dst_w = (uint32_t*)dst;
                        dst_w[i/sizeof(uint32_t)] = hb_mc_response_packet_get_data(&dst_packet);
		}
		return HB_MC_SUCCESS;	
	}
	else 
		return HB_MC_FAIL; /* invalid kind */
}

void hb_mc_cuda_sync (uint8_t fd, tile_t *tile) {
	uint8_t host_x = hb_mc_get_manycore_dimension_x() -1;
	uint8_t host_y = 0;
	hb_mc_packet_op_t op = HB_MC_PACKET_OP_REMOTE_STORE;
	hb_mc_packet_mask_t mask = HB_MC_PACKET_REQUEST_MASK_WORD; 
	hb_mc_request_packet_t finish = {host_x, host_y, tile->x, tile->y, 0x1 /* data */, mask, op, FINISH_ADDRESS, {0, 0}};
	hb_mc_device_sync(fd, &finish);
} 

void hb_mc_device_sync (uint8_t fd, hb_mc_request_packet_t *finish) {
	hb_mc_request_packet_t recv;
	/* wait for Manycore to send packet */
	while (1) {
		hb_mc_fifo_receive(fd, HB_MC_FIFO_RX_REQ, (hb_mc_packet_t *) &recv);
		
		if (hb_mc_request_packet_equals(&recv, finish) == HB_MC_SUCCESS) 
			break; /* finish packet received from Hammerblade Manycore */
	}	
}


int hb_mc_device_launch (uint8_t fd, eva_id_t eva_id, char *kernel, uint32_t argc, uint32_t argv[], char *elf, tile_t tiles[], uint32_t num_tiles) {
	eva_t args_eva;
        int error = hb_mc_device_malloc (eva_id, argc * sizeof(uint32_t), &args_eva); /* allocate device memory for arguments */
        if (error != HB_MC_SUCCESS)
            return HB_MC_FAIL;
	error = hb_mc_device_memcpy(fd, eva_id, reinterpret_cast<void *>(args_eva), (void *) &argv[0], argc * sizeof(uint32_t), hb_mc_memcpy_to_device); /* transfer the arguments to dram */
	if (error != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	
	eva_t kernel_eva; 
	error = symbol_to_eva(elf, kernel, &kernel_eva); /* get EVA of kernel */
	if (error != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	
	for (int i = 0; i < num_tiles; i++) {
		error = hb_mc_write_tile_reg(fd, eva_id, &tiles[i], ARGC_REG, argc); /* write argc to tile */
		if (error != HB_MC_SUCCESS)
			return HB_MC_FAIL; 
		
		error = hb_mc_write_tile_reg(fd, eva_id, &tiles[i], ARGV_REG, args_eva); /* write EVA of arguments to tile group */
		if (error != HB_MC_SUCCESS)
			return HB_MC_FAIL; 


		npa_t host_npa = {(uint32_t) hb_mc_get_manycore_dimension_x() - 1, 0, FINISH_ADDRESS};
		eva_t host_eva;
		error = hb_mc_npa_to_eva(eva_id, &host_npa, &host_eva); /* tile will write to this address when it finishes executing the kernel */
		if (error != HB_MC_SUCCESS)
			return HB_MC_FAIL;
		error = hb_mc_write_tile_reg(fd, eva_id, &tiles[i], SIGNAL_REG, host_eva); 
		if (error != HB_MC_SUCCESS)
			return HB_MC_FAIL;

		error = hb_mc_write_tile_reg(fd, eva_id, &tiles[i], KERNEL_REG, kernel_eva); /* write kernel EVA to tile group */
		if (error != HB_MC_SUCCESS)
			return HB_MC_FAIL; 
	} 

	return HB_MC_SUCCESS;
}


/*!
 * creates a tile group with a specified origin
 * @param[out] tiles an array of tiles that will be set in row-order. This should be allocated by the caller
 * @param[out] the number of tiles in the tile group
 * @param[in] num_tiles_x the number of columns in the tile group
 * @param[in] num_tiles_y the number of rows in the tile group
 * @param[in] origin_x the x coordinate of the tile group's origin
 * @param[in] origin_y the y coordinate of the tile group's origin 
 * */
void create_tile_group(tile_t tiles[], uint8_t num_tiles_x, uint8_t num_tiles_y, uint8_t origin_x, uint8_t origin_y) {
	/* create the tile group */
	for (uint8_t i = 0; i < num_tiles_y; i++) {
		for (uint8_t j = 0; j < num_tiles_x; j++) {
			uint32_t index = i * num_tiles_x + j;
			tiles[index].x = j + origin_x; 
			tiles[index].y = i + origin_y;
			tiles[index].origin_x = origin_x;
			tiles[index].origin_y = origin_y;
		}
	}
}

