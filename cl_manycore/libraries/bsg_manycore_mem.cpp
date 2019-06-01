#include <bsg_manycore_mem.h>

#ifdef __cplusplus
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#else
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif


/*! 
 * Copies data from Manycore to host.
 * @param x destination x coordinate
 * @param y destination y coordinate
 * @param epa tile's physical address
 * @param size number of words to copy
 * @return whether or not transaction was successful
 * */
int hb_mc_copy_from_epa (uint8_t fd, hb_mc_response_packet_t *buf, uint32_t x, uint32_t y, uint32_t epa, uint32_t size) {
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		bsg_pr_err("hb_mc_copy_from_epa(): device was not initialized.\n", __func__);
		return HB_MC_FAIL;
	}

	hb_mc_packet_t requests[size]; 	
	uint32_t base_byte = epa << 2;
	for (int i = 0; i < size; i++) {
		uint32_t addr = (base_byte + i * sizeof(uint32_t)) >> 2;
		uint32_t data = 0; /* unused */
		hb_mc_format_request_packet(fd, &requests[i].request, addr, data, x, y, HB_MC_PACKET_OP_REMOTE_LOAD);
	} 
	
	int pass_requests = HB_MC_SUCCESS; /* whether or not load requests send properly */
	for (int i = 0; i < size; i++) {
		if (hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &requests[i]) != HB_MC_SUCCESS) {
			pass_requests = HB_MC_FAIL;
			break;
		}
	}
	if (pass_requests != HB_MC_SUCCESS) {
		bsg_pr_err("%s: hb_mc_copy_from_epa(): error when sending load request to Manycore.\n", __func__);
	}
	
	/* read receive packets from Manycore. TODO: can result in infinite loop. */
	for (int i = 0; i < size; i++) {
		hb_mc_fifo_receive(fd, HB_MC_FIFO_RX_RSP, (hb_mc_packet_t *) &buf[i]);
	}
	return pass_requests;
}

/*! 
 * Copies data from host to manycore
 * @param x destination x coordinate
 * @param y destination y coordinate
 * @param epa tile's physical address
 * @param size number of words to copy
 * @return whether or not transaction was successful
 * */
int hb_mc_copy_to_epa (uint8_t fd, uint32_t x, uint32_t y, uint32_t epa, uint32_t *buf, uint32_t size) {
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: hb_xeon_to_epa_copy(): device was not initialized.\n", __func__);
		return HB_MC_FAIL;
	}
	hb_mc_packet_t packets[size];
	uint32_t base_byte = epa << 2;
	for (int i = 0; i < size; i++) {
		uint32_t addr = (base_byte + i * sizeof(uint32_t)) >> 2;
		uint32_t data = buf[i];
		hb_mc_format_request_packet(fd, &packets[i].request, addr, data, x, y, HB_MC_PACKET_OP_REMOTE_STORE);
	} 
	int pass = HB_MC_SUCCESS;
	for (int i = 0; i < size; i++) {
		if (hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &packets[i]) != HB_MC_SUCCESS) {
			pass = HB_MC_FAIL;
			break;
		}
	}
	if (pass != HB_MC_SUCCESS)
		bsg_pr_err("%s: error when writing to Manycore.\n", __func__);

	return pass;
}

/*!
 * returns HB_MC_SUCCESS if eva is a global network address and HB_MC_FAIL if not.
 */
static int hb_mc_is_global_network (eva_t eva) {
	if (hb_mc_get_bits(eva, 30, 2) == 0x1) 
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*!
 * returns HB_MC_SUCCESS if eva is a DRAM address and HB_MC_FAIL if not.
 */
static int hb_mc_eva_is_dram (eva_t eva) {
	if (hb_mc_get_bits(eva, 31, 1) == 0x1) 
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*!
 * checks if NPA is in DRAM.
 */
static int hb_mc_npa_is_dram (npa_t *npa) {
	uint8_t dim_y = hb_mc_get_manycore_dimension_y();
	if (npa->y == (dim_y + 1))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}

/*!
 * checks if NPA is in host endpoint.
 */
static int hb_mc_npa_is_host (npa_t *npa) {
	uint8_t dim_x = hb_mc_get_manycore_dimension_x();
	if ((npa->y == 0) && (npa->x == dim_x))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}

/*!
 * checks if NPA is a tile.
 */
static int hb_mc_npa_is_tile (npa_t *npa) {
	uint8_t dim_x = hb_mc_get_manycore_dimension_x();
	uint8_t dim_y = hb_mc_get_manycore_dimension_y();
	if (((npa->y >= 1) && (npa->y < dim_y)) && 
		((npa->x >= 0) && (npa->x < dim_x)))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}

/*
 * returns x coordinate of a global network address.
 */
static uint32_t hb_mc_global_network_get_x (eva_t eva) {
	return hb_mc_get_bits(eva, 18, 6); /* TODO: hardcoded */	
}

/*
 * returns y coordinate of a global network address.
 */
static uint32_t hb_mc_global_network_get_y (eva_t eva) {
	return hb_mc_get_bits(eva, 24, 6); /* TODO: hardcoded */
}

/*
 * returns x coordinate of a DRAM address.
 */
static uint32_t hb_mc_dram_get_x (eva_t eva) {
	return hb_mc_get_bits(eva, 29, 2); /* TODO: hardcoded */
}

/*
 * returns y coordinate of a DRAM address.
 */
static uint32_t hb_mc_dram_get_y (eva_t eva) {
	return hb_mc_get_manycore_dimension_y() + 1;
}


/*
 * returns EPA of a global network address.
 */
static uint32_t hb_mc_global_network_get_epa (eva_t eva) {
	return hb_mc_get_bits(eva, 0, 18) >> 2; /* TODO: hardcoded */ 
}

/*
 * returns EPA of a DRAM address.
 */
static uint32_t hb_mc_dram_get_epa (eva_t eva) {
	return hb_mc_get_bits(eva, 2, 27); /* TODO: hardcoded */ 
}

/*!
 * Converts an EVA address to an NPA address.
 * @param eva_id specifies EVA-NPA mapping.
 * @param eva EVA address
 * @param npa pointer to npa_t object where NPA address should be stored.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * This function only supports DRAM and Global Network Address EVAs.
 */
int hb_mc_eva_to_npa_deprecated (eva_id_t eva_id, eva_t eva, npa_t *npa) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid eva_id */
	}
	else if (hb_mc_is_global_network(eva) == HB_MC_SUCCESS) {
		uint32_t x = hb_mc_global_network_get_x(eva);
		uint32_t y = hb_mc_global_network_get_y(eva);
		uint32_t epa = hb_mc_global_network_get_epa(eva);
		*npa = {x, y, epa};
	}
	else if (hb_mc_eva_is_dram(eva) == HB_MC_SUCCESS) {
		uint32_t x = hb_mc_dram_get_x(eva);	
		uint32_t y = hb_mc_dram_get_y(eva);
		uint32_t epa = hb_mc_dram_get_epa(eva);
		*npa = {x, y, epa};
	}
	else {
		return HB_MC_FAIL; /* invalid EVA */
	}
	return HB_MC_SUCCESS;
}


/*!
 * Checks if a Vanilla Core EPA is valid.
 * @param epa.
 * @return HB_MC_SUCCESS if the EPA is valid and HB_MC_FAIL if the EPA is invalid.
 * */
static int hb_mc_valid_epa_tile (uint32_t epa) {
	if (epa >= 0x1000 && epa <= 0x1FFF) /* TODO: hardcoded */
		return HB_MC_SUCCESS; /* data memory */
	else if (epa >= 0x1000000 && epa <= 0x1FFEFFF)	/* TODO: hardcoded */
		return HB_MC_SUCCESS; /* instruction cache */
	else if (epa == 0x20000) /* TODO: hardcoded */
		return HB_MC_SUCCESS; /* FREEZE CSR */
	else if (epa == 0x20004) /* TODO: hardcoded */
		return HB_MC_SUCCESS; /* Tile Group Origin X Cord CSR */
	else if (epa == 0x20008) /* TODO: hardcoded */
		return HB_MC_FAIL; /* Tile Group Origin Y Cord CSR */
} 

/*!
 * Checks if a DRAM EPA is valid.
 * @param epa.
 * @return HB_MC_SUCCESS if the EPA is valid and HB_MC_FAIL if the EPA is invalid.
 * */
static int hb_mc_valid_epa_dram (uint32_t epa) {
	uint32_t dram_size = (1 << 27) - 1; /* TODO: hardcoded */
	uint32_t dram_size_words = dram_size >> 2;
	if (epa <= dram_size_words)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;

}

/*!
 * checks if NPA has valid (x,y) coordinates. 
 */
static int hb_mc_npa_is_valid (npa_t *npa) {
	if (hb_mc_npa_is_dram(npa) == HB_MC_SUCCESS && hb_mc_valid_epa_dram(npa->epa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* valid DRAM NPA */
	else if (hb_mc_npa_is_host(npa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* for now, we assume any EPA is valid for the host */
	else if (hb_mc_npa_is_tile(npa) == HB_MC_SUCCESS && hb_mc_valid_epa_tile(npa->epa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* valid Vanilla Core NPA */
	else 
		return HB_MC_FAIL;
}

/*! creates a NPA to DRAM EVA.
 * @param[in] npa Caller should ensure that this is valid.
 */
static eva_t hb_mc_npa_to_eva_dram(const npa_t *npa) {
	eva_t eva = 0;
	eva |= (npa->epa << 2);
	eva |= (npa->x << (2 + 27)); /* TODO: hardcoded */
	eva |= (1 << 31); /* TODO: hardcoded */
	return eva;
}

/*! converts NPA to Global Remote EVA.
 * @param[in] npa Caller should ensure that this is valid.
 */
static eva_t hb_mc_npa_to_eva_global_remote(const npa_t *npa) {
	eva_t eva = 0;
	eva |= (npa->epa << 2);
	eva |= (npa->x << 18); /* TODO: hardcoded */
	eva |= (npa->y << 24); /* TODO: hardcoded */
	eva |= (1 << 30); /* TODO: hardcoded */
	return eva;
}

/*!
 * Converts an NPA to an EVA. 
 * @param eva_id specified EVA-NPA mapping.
 * @param npa pointer to npa_t struct to convert.
 * @param eva pointer to an eva_t that this function should set.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. This function will fail if the NPA is invalid.
 */
int hb_mc_npa_to_eva_deprecated (eva_id_t eva_id, npa_t *npa, eva_t *eva) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid eva_id */
	}
	else if (hb_mc_npa_is_valid(npa) != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* invalid NPA address*/
	}
	else if (hb_mc_npa_is_dram(npa) == HB_MC_SUCCESS) {
		*eva = hb_mc_npa_to_eva_dram(npa);
	}
	else { /* tile or host endpoint */
		*eva = hb_mc_npa_to_eva_global_remote(npa);
	}
	return HB_MC_SUCCESS;
}
