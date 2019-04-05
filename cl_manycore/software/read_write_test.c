#include "read_write_test.h"

int read_write_test() {
		
	int rc; // return code
	rc = fpga_pci_init();
	if (rc != 0)
		return -1;
	
	void print_hex(uint8_t  *p) {
		for (int i = 0; i < 16; i++) {
			printf("%x ", (p[15-i] & 0xFF));
		}
		printf("\n");
	}

	uint8_t fd; 
	hb_mc_init_host(&fd);	
	
	/* store data in tile */
	uint32_t data = 0xABCD;
	printf("write to DMEM\n");
	int write = hb_mc_copy_to_epa(0, 0, 1, DMEM_BASE >> 2, &data, 1);
	
	if (write != HB_MC_SUCCESS) {
		printf("writing data to tile (0, 0)'s DMEM failed.\n");
		return -1;
	}
	printf("write success\n");
	/* read back data */
	request_packet_t buf[1];
	int read = hb_mc_copy_from_epa(fd, (request_packet_t *) &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
	printf("completed read.\n");
	if (read == HB_MC_SUCCESS) {
		printf("read packet: ");
		print_hex((uint8_t *) &buf[0]);
		return 0;
	}	
	else {
		printf("read from tile failed.\n");
		return -1;
	}
}


#ifdef COSIM
	void test_main(uint32_t *exit_code) {
		int rc;
		rc = read_write_test();
		*exit_code = rc;
		return;
	}
#else
	int main() {
		return read_write_test();
	}
#endif
