#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_tile.h>

#include <sys/stat.h>

#include "test_bsg_loader_suite.h"

#define SUITE_NAME "test_bsg_loader_suite"

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))
/*
   Reads program data from a file into a buffer that read_program_file() allocates.
   Returns the data and size.
*/
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

// /path/to/spmd/bsg_loader_suite
#define SUITE_DIR BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_loader_suite"

/* our test structures */
typedef struct test {
        const char *name;
#define TN_FMT "15s"
        const char *program_file_name;
        hb_mc_manycore_t *mc;
        uint32_t magic; // used in finish packet test
} test_t;

#define pr_test_failed(test, reason, ...)                               \
        bsg_pr_test_info("%s: %" TN_FMT ": " BSG_RED("FAILED") ": " reason, \
                         SUITE_NAME,                                    \
                         test->name,                                    \
                         ##__VA_ARGS__                                  \
                )

#define pr_test_passed(test, reason, ...)                                \
        bsg_pr_test_info("%s: %" TN_FMT ": " BSG_GREEN("PASSED") ": " reason, \
                         SUITE_NAME,                                    \
                         test->name,                                    \
                         ##__VA_ARGS__                                  \
                )


static hb_mc_manycore_t manycore = {0}; // the manycore to instantiate

/*******************************************************************************/
/* Tests that should successfully load a file and receive a finish/fail packet */
/*******************************************************************************/
#define FINISH_PACKET_TEST(tname, tfile)                                  \
        { .name = tname, .program_file_name = tfile, .mc = &manycore, .magic = 0x3AB4 }

#define FAIL_PACKET_TEST(tname, tfile)          \
        { .name = tname, .program_file_name = tfile, .mc = &manycore, .magic = 0x3AB6 }

/**************************************/
/* ADD NEW PACKET LOOPBACK TESTS HERE */
/**************************************/
static test_t finish_packet_tests [] = {
        FINISH_PACKET_TEST("loopback",          SUITE_DIR "/loopback/main.riscv"),
        FINISH_PACKET_TEST("loopback cache",    SUITE_DIR "/loopback_cache/main.riscv"),
        FAIL_PACKET_TEST(  "loopback fail",     SUITE_DIR "/loopback_fail/main.riscv"),
        FINISH_PACKET_TEST("loopback big text", SUITE_DIR "/loopback_big_text/main.riscv"),
};

static int run_finish_packet_test(test_t *test)
{
        hb_mc_manycore_t *mc = test->mc;
        const char *program_file_name = test->program_file_name;
        const char *test_name = test->name;
        unsigned char *program_data;
        size_t program_size;
        int rc = HB_MC_FAIL, err;

        rc = read_program_file(program_file_name, &program_data, &program_size);
        if (rc != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to read program data\n");
                return rc;
        }
	/* initialize the tile */
	hb_mc_coordinate_t target = hb_mc_coordinate(0,1);
	hb_mc_coordinate_t origin = hb_mc_coordinate(0,1);

	// freeze the tile
	err = hb_mc_tile_freeze(mc, &target);
	if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to freeze tile (%" PRId32 ",%" PRId32"): %s\n",
                               hb_mc_coordinate_get_x(target),
                               hb_mc_coordinate_get_y(target),
                               hb_mc_strerror(err)
                        );
                goto cleanup;
	}

	// set its origin
	err = hb_mc_tile_set_origin(mc, &target, &origin);
	if (err != HB_MC_SUCCESS) {
		pr_test_failed(test, "failed to set origin of (%" PRId32 ", %" PRId32 ") "
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
		pr_test_failed(test, "failed to load binary '%s': %s\n",
			   program_file_name,
			   hb_mc_strerror(err));
		return err;
	}

	err = hb_mc_tile_unfreeze(mc, &target);
	if (err != HB_MC_SUCCESS) {
		pr_test_failed(test, "failed to unfreeze tile (%" PRId32", %" PRId32 "): %s\n",
                               hb_mc_coordinate_get_x(target),
                               hb_mc_coordinate_get_y(target),
                               hb_mc_strerror(err));
		goto cleanup;
	}

	usleep(100);

        /* wait for the finish packet */
	hb_mc_packet_t finish;
	err = hb_mc_manycore_packet_rx(mc, &finish, HB_MC_FIFO_RX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		pr_test_failed(test, "failed to receive packet: %s\n",
                               hb_mc_strerror(err));
		goto cleanup;
	}


        /* print out the packet */
	char buf[256];
	bsg_pr_test_info("%s: %" TN_FMT ": received manycore finish packet %s\n",
			 SUITE_NAME,
                         test->name,
                         hb_mc_request_packet_to_string(&finish.request, buf ,sizeof(buf)));


	uint32_t magic = test->magic;
	if (hb_mc_request_packet_get_addr(&finish.request) != magic) {
		pr_test_failed(test, "packet does not match finish packet 0x%08" PRIx32 "\n",
                               magic);
		goto cleanup;
	} else {
                pr_test_passed(test, "received correct packet!\n");
        }

        rc = HB_MC_SUCCESS;
cleanup:
        free(program_data);
done:
        return rc;
}

/*
   Run all off the tests that should succesfully load a program and receive
   a finish packet from the hardware.
*/
static int run_finish_packet_tests(void)
{
        int err, rc = HB_MC_SUCCESS;
        for (int i = 0; i < array_size(finish_packet_tests); i++) {
                err = run_finish_packet_test(&finish_packet_tests[i]);
                if (err != HB_MC_SUCCESS)
                        rc = HB_MC_FAIL;
                // continue running all tests
        }
        return rc;
}


/******************/
/* Main test loop */
/******************/
int test_loader_suite (void) {
        int err, rc = HB_MC_SUCCESS;

        err = hb_mc_manycore_init(&manycore, SUITE_NAME, 0);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to initialize manycore: %s\n",
                             SUITE_NAME, hb_mc_strerror(err));
                return HB_MC_FAIL;
        }

        err = run_finish_packet_tests();
        if (err != HB_MC_SUCCESS)
                rc = HB_MC_FAIL;

        hb_mc_manycore_exit(&manycore);

        return err;
}


#ifdef COSIM
void test_main(uint32_t *exit_code) {
	bsg_pr_test_info(SUITE_NAME " Regression Test (COSIMULATION)\n");
	int rc = test_loader_suite();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info(SUITE_NAME " Regression Test (F1)\n");
	int rc = test_loader_suite();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

