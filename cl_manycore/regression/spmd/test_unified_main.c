#include "test_unified_main.h"
#include <sys/stat.h>

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

int test_unified_main (int argc, char **argv) {
	unsigned char *program_data;
	size_t program_size;
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;
	int err, r = HB_MC_FAIL, len;

	const char *spmd_path = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/";
	const char *spmd_name = "/main.riscv";
        char *test_name;
        struct arguments args = {NULL};

        argp_parse (&argp_name, argc, argv, 0, 0, &args);
        test_name = args.testname;
        bsg_pr_test_info("%s Regression Test\n", test_name);

	len = strlen(spmd_path) + strlen(test_name) + strlen(spmd_name);

	char program_path[len + 1], *ptr;
	program_path[len] = '\0';
	ptr = program_path;

	strcpy(ptr, spmd_path);
	ptr += strlen(spmd_path);

	test_name += strlen("test_");
	strcpy(ptr, test_name);
	ptr += strlen(test_name);

	strcpy(ptr, spmd_name);

	bsg_pr_test_info("Reading from file: %s\n", program_path);

	// read in the program data from the file system
	err = read_program_file(program_path, &program_data, &program_size);
	if (err != HB_MC_SUCCESS)
		return err;

	// initialize the manycore
	err = hb_mc_manycore_init(&manycore, test_name, 0);
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
			   program_path,
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

	while (true) {
		hb_mc_packet_t pkt;
		int err;
		bsg_pr_dbg("Waiting for finish packet\n");
		
		err = hb_mc_manycore_packet_rx(mc, &pkt, HB_MC_FIFO_RX_REQ, -1);
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("failed to read response packet: %s\n",
				   hb_mc_strerror(err));
		
			return HB_MC_FAIL;
		}

		char pkt_str[128];
		hb_mc_request_packet_to_string(&pkt.request, pkt_str, sizeof(pkt_str));

		bsg_pr_dbg("received packet %s\n", pkt_str);
		
		switch (hb_mc_request_packet_get_epa(&pkt.request)) {
		case 0xEAD0:
			bsg_pr_dbg("received finish packet\n");
			err = HB_MC_SUCCESS;
			goto cleanup;
		case 0xEAD8:
			bsg_pr_dbg("received fail packet\n");
			err = HB_MC_FAIL;
			goto cleanup;
		default: break;
		}
	}
cleanup:
	hb_mc_manycore_exit(mc);
	return err;
	
}

#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
        // We aren't passed command line arguments directly so we parse them
        // from *args. args is a string from VCS - to pass a string of arguments
        // to args, pass c_args to VCS as follows: +c_args="<space separated
        // list of args>"
        int argc = get_argc(args);
        char *argv[argc];
        get_argv(args, argc, argv);

#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	int rc = test_unified_main(argc, argv);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char ** argv) {
	int rc = test_unified_main(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif

