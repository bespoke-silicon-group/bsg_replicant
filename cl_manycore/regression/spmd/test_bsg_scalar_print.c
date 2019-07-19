#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_tile.h>

#include <sys/stat.h>

#include "test_bsg_scalar_print.h"

#define TEST_NAME "test_bsg_scalar_print"

#define MAGIC_INT    1234
#define MAGIC_UINT   99220011
#define MAGIC_HEX    0xA0A0
#define MAGIC_FLOAT  1843.50
#define MAGIC_SCI    2.25

typedef union { float f; unsigned u; } utof_t;

static int read_program_file(const char *file_name, unsigned char **file_data, size_t *file_size)
{
	struct stat st;
	FILE *f;
	int r;
	unsigned char *data;

	if ((r = stat(file_name, &st)) != 0) {
		bsg_pr_err("could not stat '%s': %m\n", file_name);
		return HB_MC_FAIL;
	}

	if (!(f = fopen(file_name, "rb"))) {
		bsg_pr_err("failed to open '%s': %m\n", file_name);
		return HB_MC_FAIL;
	}

	if (!(data = (unsigned char *) malloc(st.st_size))) {
		bsg_pr_err("failed to read '%s': %m\n", file_name);
		fclose(f);
		return HB_MC_FAIL;
	}

	if ((r = fread(data, st.st_size, 1, f)) != 1) {
		bsg_pr_err("failed to read '%s': %m\n", file_name);
		fclose(f);
		free(data);
		return HB_MC_FAIL;
	}

	fclose(f);
	*file_data = data;
	*file_size = st.st_size;
	return HB_MC_SUCCESS;
}

int test_scalar_print () {
	unsigned char *program_data;
	size_t program_size;
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;
	int err, r = HB_MC_FAIL;


	const char *program_file_name = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/"
		"bsg_scalar_print/main.riscv";

	// read in the program data from the file system
	err = read_program_file(program_file_name, &program_data, &program_size);
	if (err != HB_MC_SUCCESS)
		return err;

	// initialize the manycore
	err = hb_mc_manycore_init(&manycore, TEST_NAME, 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to initialize manycore instance: %s\n",
			   hb_mc_strerror(err));
		return err;
	}

	/* initialize the tile */
	hb_mc_coordinate_t target = hb_mc_coordinate(0,1);
	hb_mc_coordinate_t origin = hb_mc_coordinate(0,1);

	// freeze the tile
	err = hb_mc_tile_freeze(mc, &target);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to freeze tile (%" PRId32 ", %" PRId32 "): %s\n",
			   hb_mc_coordinate_get_x(target),
			   hb_mc_coordinate_get_y(target),
			   hb_mc_strerror(err));
		goto cleanup;
	}

	// set its origin
	err = hb_mc_tile_set_origin(mc, &target, &origin);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to set origin of (%" PRId32 ", %" PRId32 ") "
			   "to (%" PRId32 ", %" PRId32 "): %s\n",
			   hb_mc_coordinate_get_x(target),
			   hb_mc_coordinate_get_y(target),
			   hb_mc_coordinate_get_x(origin),
			   hb_mc_coordinate_get_y(origin),
			   hb_mc_strerror(err));
		goto cleanup;
	}

	/* load the program */
	err = hb_mc_loader_load(program_data, program_size,
				mc, &default_map,
				&target, 1);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to load binary '%s': %s\n",
			   program_file_name,
			   hb_mc_strerror(err));
		return err;
	}

	err = hb_mc_tile_unfreeze(mc, &target);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to unfreeze tile (%" PRId32", %" PRId32 "): %s\n",
			   hb_mc_coordinate_get_x(target),
			   hb_mc_coordinate_get_y(target),
			   hb_mc_strerror(err));
		goto cleanup;
	}

	usleep(100);

	bsg_pr_test_info("Checking receive packets:\n");
	char buf[256];
	hb_mc_packet_t recv, finish;
	utof_t f_data;

	// Receive bsg_print_int packet
	err = hb_mc_manycore_packet_rx(mc, &recv, HB_MC_FIFO_RX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to receive packet: %s\n",
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Received manycore bsg_print_int packet %s\n",
			 hb_mc_request_packet_to_string(&recv.request, buf ,sizeof(buf)));
	if (hb_mc_request_packet_get_data(&recv.request) == MAGIC_INT) {
		bsg_pr_test_info(BSG_GREEN("Packet Match: ") "Expected %d -- Received %d\n",
				  MAGIC_INT, hb_mc_request_packet_get_data(&recv.request));
	}
	else {
		bsg_pr_test_info(BSG_RED("Packet Mismatch: ") "Expected %d -- Received %d\n",
				  MAGIC_INT, hb_mc_request_packet_get_data(&recv.request));
		goto cleanup;
	}


	// Receive bsg_print_unsigned packet
	err = hb_mc_manycore_packet_rx(mc, &recv, HB_MC_FIFO_RX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to receive packet: %s\n",
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Received manycore bsg_print_unsigned packet %s\n",
			 hb_mc_request_packet_to_string(&recv.request, buf ,sizeof(buf)));
	if (hb_mc_request_packet_get_data(&recv.request) == MAGIC_UINT) {
		bsg_pr_test_info(BSG_GREEN("Packet Match: ") "Expected %u -- Received %u\n",
				  MAGIC_UINT, hb_mc_request_packet_get_data(&recv.request));
	}
	else {
		bsg_pr_test_info(BSG_RED("Packet Mismatch: ") "Expected %u -- Received %u\n",
				  MAGIC_UINT, hb_mc_request_packet_get_data(&recv.request));
		goto cleanup;
	}


	// Receive bsg_print_hex packet
	err = hb_mc_manycore_packet_rx(mc, &recv, HB_MC_FIFO_RX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to receive packet: %s\n",
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Received manycore bsg_print_hex packet %s\n",
			 hb_mc_request_packet_to_string(&recv.request, buf ,sizeof(buf)));
	if (hb_mc_request_packet_get_data(&recv.request) == MAGIC_HEX) {
		bsg_pr_test_info(BSG_GREEN("Packet Match: ") "Expected 0x%08" PRIx32 " -- Received 0x%08" PRIx32 "\n",
				  MAGIC_HEX, hb_mc_request_packet_get_data(&recv.request));
	}
	else {
		bsg_pr_test_info(BSG_RED("Packet Mismatch: ") "Expected 0x%08" PRIx32 " -- Received 0x%08" PRIx32 "\n",
				  MAGIC_HEX, hb_mc_request_packet_get_data(&recv.request));
		goto cleanup;
	}


	// Receive bsg_print_float packet
	err = hb_mc_manycore_packet_rx(mc, &recv, HB_MC_FIFO_RX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to receive packet: %s\n",
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Received manycore bsg_print_float packet %s\n",
			 hb_mc_request_packet_to_string(&recv.request, buf ,sizeof(buf)));
	f_data.u = hb_mc_request_packet_get_data(&recv.request);
	if (f_data.f  == MAGIC_FLOAT) {
		bsg_pr_test_info(BSG_GREEN("Packet Match: ") "Expected %f -- Received %f\n",
				  MAGIC_FLOAT, f_data.f);
	}
	else {
		bsg_pr_test_info(BSG_RED("Packet Mismatch: ") "Expected %f -- Received %f\n",
				  MAGIC_FLOAT, f_data.f);
		goto cleanup;
	}


	// Receive bsg_print_float_scientific packet
	err = hb_mc_manycore_packet_rx(mc, &recv, HB_MC_FIFO_RX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to receive packet: %s\n",
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Received manycore bsg_print_float_scientific packet %s\n",
			 hb_mc_request_packet_to_string(&recv.request, buf ,sizeof(buf)));
	f_data.u = hb_mc_request_packet_get_data(&recv.request);
	if (f_data.f  == MAGIC_SCI) {
		bsg_pr_test_info(BSG_GREEN("Packet Match: ") "Expected %e -- Received %e\n",
				  MAGIC_SCI, f_data.f);
	}
	else {
		bsg_pr_test_info(BSG_RED("Packet Mismatch: ") "Expected %e -- Received %e\n",
				  MAGIC_SCI, f_data.f);
		goto cleanup;
	}


	// success
	r = HB_MC_SUCCESS;

cleanup:
	hb_mc_manycore_exit(mc);
	return r;
}


#ifdef COSIM
void test_main(uint32_t *exit_code) {
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_bsg_scalar_print Regression Test (COSIMULATION)\n");
	int rc = test_scalar_print();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_bsg_scalar_print Regression Test (F1)\n");
	int rc = test_scalar_print();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

