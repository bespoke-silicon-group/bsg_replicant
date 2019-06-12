#include "bsg_manycore_run_spmd_program.hpp"
#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <cstdlib>
#include <cstdio>
#include <getopt.h>

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

static void usage()
{
	pr_usage("usage: %s [OPTIONS] PROGRAM-FILE\n", execname);

	pr_usage("arguments:\n");
	argument_usage("PROGRAM-FILE", "Your SPMD Manycore Program");

	pr_usage("options:\n");
	option_usage("-h,--help", "Print this help message");    
}

int main(int argc, char *argv[])
{
	static struct option options [] = {
		{ "help" , 0 , 0, 'h' },
		{ /* sentinel */ }
	};
	static const char * opstring = "h";
	const char *program_file_name;
	int ch;
	
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
		bsg_pr_err("Missing a program argument\n");
		usage();
		exit(1);
	}

	program_file_name = argv[0];

	return bsg_manycore_run_spmd_program(program_file_name);	
}
