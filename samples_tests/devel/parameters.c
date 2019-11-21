#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "parameters.h"

static void usage()
{
	puts(	"Tool to display parameters.\n"
		"\n"
		"  -h   Display this help screen.\n"
		"  -K # Set K value.\n"
	);
}

static void print_param(void* usr, const char* p_name, int p_val)
{
	printf("  %s = %d\n", p_name, p_val);
}

int main(int argc, char** argv)
{
	int K = 60;

	/* Read command line args */
	int c;
	while ((c = getopt(argc, argv, "hK:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'K':
			K = atoi(optarg);
			break;
		case '?':
			return EXIT_FAILURE;
		};
	}

	printf("Input K = %d\n", K);

	parameters P = parameters_get(K);
	puts("Got the following values:");
	parameters_dump(&P, NULL, print_param);

	return 0;
}
