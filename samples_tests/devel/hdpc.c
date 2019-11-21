#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "hdpc.h"
#include "m256v.h"
#include "parameters.h"

static void usage()
{
	puts(	"Tool to display a HDPC matrix.\n"
		"\n"
		"  -h   Display this help screen.\n"
		"  -K # Set K value.\n"
		"  -f   Enable fast algorithm."
	);
}

int main(int argc, char** argv)
{
	int K = 60;
	int use_faster = 0;

	/* Read command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hK:f")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'K':
			K = atoi(optarg);
			break;
		case 'f':
			use_faster = 1;
			break;
		case '?':
			return EXIT_FAILURE;
		};
	}

	/* Get parameters */
	parameters P = parameters_get(K);
	if (P.K == -1) {
		fprintf(stderr, "Error:  Invalid choice of K.\n");
		return 1;
	}

	/* Generate a suitable LDPC matrix */
	uint8_t* m = malloc(P.H * P.L * sizeof(uint8_t));
	m256v H = m256v_make(P.H, P.L, m);
	if (use_faster) {
		hdpc_generate_mat_faster(&H, &P);
	} else {
		hdpc_generate_mat_specexact(&H, &P);
	}

	/* Print on screen */
	for (int r = 0; r < P.H; ++r) {
		for (int c = 0; c < P.L; ++c) {
			const uint8_t v = m256v_get_el(&H, r, c);
			printf(" %02x", (int)v);
		}
		fputc('\n', stdout);
	}

	return EXIT_SUCCESS;
}
