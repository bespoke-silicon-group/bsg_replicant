#include "cl_manycore_test.h"

#ifdef COSIM
	// this method is defined by aws' utils/sh_dpi_tasks.h
	void test_main(uint32_t *exit_code) {
#else
	int main() {
#endif

#ifdef COSIM	
		printf("Running the Cosimulation Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");
#else	
		printf("Running the Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");
#endif


		// below is the user defined test task

		int rc; // return code
		rc = fpga_pci_init();
		return_code(rc, exit, "unable to initialize the fpga_pci library");
		
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
			rc = -1;
			return_code(rc, exit, "xxxx");
		}
		printf("write success\n");
		/* read back data */
		request_packet_t buf[1];
		int read = hb_mc_copy_from_epa(fd, (request_packet_t *) &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
		printf("completed read.\n");
		if (read == HB_MC_SUCCESS) {
			printf("read packet: ");
			print_hex((uint8_t *) &buf[0]);
			rc = 0;
			return_code(rc, exit, "xxx");
		}	
		else {
			printf("read from tile failed.\n");
			rc = -1;
			return_code(rc, exit, "xxxx");
		}
exit:
#ifdef COSIM
		*exit_code = rc;
		return;
#else
		return rc;
#endif
}

