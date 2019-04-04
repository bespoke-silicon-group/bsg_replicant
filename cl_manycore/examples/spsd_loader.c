#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>

#include <bsg_manycore_driver.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_packet.h>
#include <bsg_manycore_errno.h>

#define pr_usage(fmt, ...)			\
    fprintf(stderr, fmt, ##__VA_ARGS__)

static const char *execname = "";

static void base_usage(const char *name, const char *help)
{
    pr_usage("\t%s\t%s\n", name, help);
}

static void argument_usage(const char *argname, const char *arghelp)
{
    base_usage(argname, arghelp);
}

static void option_usage(const char *opname, const char *ophelp)
{
    base_usage(opname, ophelp);
}

static uint8_t x = 0, y = 1;
static void usage()
{
    pr_usage("usage: %s [OPTIONS] RISCV-EXECUTABLE\n", execname);

    pr_usage("This program loads and runs a single threaded-manycore program on tile (%d,%d)", x, y);
    
    pr_usage("arguments:\n");
    argument_usage("RISCV-EXECUTABLE", "A manycore binary");

    pr_usage("options:\n");
    option_usage("-h,--help", "Print this help message");    
}

int main(int argc, char *argv[])
{
    static struct option options [] = {
	{ "help" ,   0 , 0, 'h' },
	{ /* sentinal */ }
    };
    static const char * opstring = "h";
    int ch;

    int rows = 1, columns = 1;
    
    execname = argv[0];

    // process options
    while ((ch = getopt_long(argc, argv, opstring, options, NULL)) != -1) {
	switch (ch) {
	case 'h':
	    usage();
	    exit(0);
	default:
	    usage();
	    exit(1);
	}
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
	fprintf(stderr, "error: no executable given\n");
	usage();
	exit(1);
    }
    
    uint8_t fd;
    char *path_to_manycore_program = argv[0]; /* Put path to the riscv file here */

    if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
	printf("failed to initialize host.\n");
	return 0;
    }

    hb_mc_freeze(fd, x, y);
	
    hb_mc_load_binary(fd, path_to_manycore_program, &x, &y, 1);

    hb_mc_unfreeze(fd, x, y);

    /* Assuming that the manycore program sends a packet to the host, this code attempts to read the packet */
    printf("Checking receive packet...\n");
    usleep(100); /* 100 us */

    hb_mc_request_packet_t rqst;
    int r;
    
    while (1) {
	hb_mc_request_packet_t rqst;
	hb_mc_read_fifo(fd, 1, &rqst);
	if (hb_mc_request_packet_get_addr(&rqst) == (0xEADC >> 2)) {
	    putchar(hb_mc_request_packet_get_data(&rqst));
	} else if (hb_mc_request_packet_get_addr(&rqst) == (0xEAD0 >> 2)) {
	    r = 0;
	    break;
	} else if (hb_mc_request_packet_get_addr(&rqst) == (0xEAD4 >> 2)) {
	    // I guess print the time?
	} else if (hb_mc_request_packet_get_addr(&rqst) == (0xEAD8 >> 2)) {
	    r = 1;
	    break;
	}
    }

    hb_mc_freeze(fd, x, y);
    
    return r;
}
