#include "../deploy.h"
#include "../loader/spmd_loader.h"

/*! 
 * Copies data from Manycore to host.
 * @param x destination x coordinate
 * @param y destination y coordinate
 * @param epa tile's physical address
 * @param size number of words to copy
 * @return whether or not transaction was successful
 * */
bool hb_epa_to_xeon_copy (uint32_t **buf, uint32_t x, uint32_t y, uint32_t epa, uint32_t size) {
//	if (!can_read(size)) {
//		printf("hb_xeon_to_copy(): not enough space to read.\n");
//		return false;
//	}

	uint8_t **packets = calloc(size, sizeof(uint8_t *));
	for (int i = 0; i < size; i++) {
		packets[i] = get_pkt(epa, 0, x, y, OP_REMOTE_LOAD);
	} 
	
	bool pass_requests = true; /* whether or not load requests send properly */
	for (int i = 0; i < size; i++) {
		if (!deploy_write_fifo(0, (uint32_t *) packets[i])) {
			pass_requests = false;
			break;
		}
	}

	if (!pass_requests)
		printf("hb_xeon_to_epa_copy(): error when sending load request to Manycore.\n");

	/* read receive packets from Manycore. TODO: can result in infinite loop. */
	for (int i = 0; i < size; i++) {
		buf[i] = deploy_read_fifo(0, NULL);
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
bool hb_xeon_to_epa_copy(uint32_t x, uint32_t y, uint32_t epa, uint32_t *buf, uint32_t size) {
	uint8_t **packets = calloc(size, sizeof(uint8_t *));
	for (int i = 0; i < size; i++) {
		packets[i] = get_pkt(epa, buf[i], x, y, OP_REMOTE_STORE);
	} 
	
	bool pass = true;
	for (int i = 0; i < size; i++) {
		if (!deploy_write_fifo(0, (uint32_t *) packets[i])) {
			pass = false;
			break;
		}
	}

	if (!pass)
		printf("hb_xeon_to_epa_copy(): error when writing to Manycore.\n");

	return pass;

}
/*
uint8_t **hb_npa_to_xeon_copy(uint32_t *buf, uint64_t npa, uint32_t size) {
}

uint8_t **hb_xeon_to_npa_copy(uint64_t npa, uint32_t *buf, uint32_t size) {


}
*/
