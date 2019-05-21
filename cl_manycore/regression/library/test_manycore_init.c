#include <bsg_manycore_printing.h>
#include <bsg_manycore.h>
#include "test_manycore_init.h"

#define TEST_NAME "test_manycore_init"

#define test_pr_err(fmt, ...)				\
    bsg_pr_err(TEST_NAME ": " fmt, ##__VA_ARGS__)

#define test_pr_info(fmt, ...)				\
    bsg_pr_info(TEST_NAME ": " fmt, ##__VA_ARGS__)

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

typedef enum mcptrval {
    MANYCORE_PTR_DEFAULT = 0,
    MANYCORE_PTR_NULL,
    MANYCORE_PTR_INITIALIZED,
} manycore_ptr_value;

static const char * manycore_ptr_value_to_string(manycore_ptr_value ptrval)
{
    static const char *strtab [] = {
	[MANYCORE_PTR_DEFAULT]     = "uninitialized",
	[MANYCORE_PTR_NULL]        = "null",
	[MANYCORE_PTR_INITIALIZED] = "initialized",
    };

    return strtab[ptrval];
}

struct test {
    const char *name;
    struct {
	manycore_ptr_value mc_ptr;
	const char *mc_name;
	int mc_id;
    } manycore_init_input;
    struct {
	int mc_err;
    } manycore_init_output;
};

/*******************************/
/* Add new tests to this array */
/*******************************/
static struct test tests [] = {
    {
	.name = "nullptr-input",
	.manycore_init_input  = { MANYCORE_PTR_NULL, "null", 0},
	.manycore_init_output = { HB_MC_INVALID },
    },
    {
	.name = "non-zero-id",
	.manycore_init_input  = { MANYCORE_PTR_DEFAULT, "nzero", 1},
	.manycore_init_output = { HB_MC_INVALID },
    },
    {
	.name = "no-name",
	.manycore_init_input = { MANYCORE_PTR_DEFAULT, 0, 0},
	.manycore_init_output = { HB_MC_INVALID },
    },
    {
	.name = "init-twice",
	.manycore_init_input = { MANYCORE_PTR_INITIALIZED, "twice", 0},
	.manycore_init_output = { HB_MC_INITIALIZED_TWICE },
    },
    {
	.name = "good-input",
	.manycore_init_input = { MANYCORE_PTR_DEFAULT, "good", 0},
	.manycore_init_output = { HB_MC_SUCCESS },
    },
};

static
int test_manycore_init(void)
{
    int testno, fail = 0, err;

    hb_mc_manycore_t initialized;
    err = hb_mc_manycore_init(&initialized, "initialized", 0);
    if (err != HB_MC_SUCCESS) {
	test_pr_err(BSG_RED("ERROR") " while initializing test suite: %s\n",
		    hb_mc_strerror(err));
	return err;
    }
    
    // for each test
    for (testno = 0; testno < array_size(tests); testno++) {	
	struct test *test = &tests[testno];
	hb_mc_manycore_t manycore = {0};

	switch (test->manycore_init_input.mc_ptr) {
	case MANYCORE_PTR_NULL:
	    err = hb_mc_manycore_init(0,
				      test->manycore_init_input.mc_name,
				      test->manycore_init_input.mc_id);
	    break;
	case MANYCORE_PTR_INITIALIZED:
	    err = hb_mc_manycore_init(&initialized,
				      test->manycore_init_input.mc_name,
				      test->manycore_init_input.mc_id);
	    break;	    
	case MANYCORE_PTR_DEFAULT:
	default:
	    err = hb_mc_manycore_init(&manycore,
				      test->manycore_init_input.mc_name,
				      test->manycore_init_input.mc_id);
	    break;
	}
	
	const char *status = (err == test->manycore_init_output.mc_err
			      ? BSG_GREEN("PASSED")
			      : BSG_RED("FAILED"));

	fail = fail || (err == test->manycore_init_output.mc_err);
	
	// determine if the test passed or failed
	test_pr_info("Test %15s %s: "
		     "Called hb_mc_manycore_init(%s,%s,%d), "
		     "Expected %s, "
		     "Returned %s\n",
		     test->name,
		     status,
		     manycore_ptr_value_to_string(test->manycore_init_input.mc_ptr),
		     test->manycore_init_input.mc_name,
		     test->manycore_init_input.mc_id,
		     hb_mc_strerror(test->manycore_init_output.mc_err),
		     hb_mc_strerror(err));
	
	// cleanup if init succeeded
	if (err == HB_MC_SUCCESS &&
	    (test->manycore_init_input.mc_ptr != MANYCORE_PTR_INITIALIZED))	    
	    hb_mc_manycore_exit(&manycore);
    }
    
    return fail ? HB_MC_FAIL : HB_MC_SUCCESS;
}
#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info(TEST_NAME " Regression Test (COSIMULATION)\n");
	int rc = test_manycore_init();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info(TEST_NAME " Regression Test (F1)\n");
	int rc = test_manycore_init();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
