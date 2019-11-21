#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "ldpc.h"
#include "m256v.h"
#include "parameters.h"

static void usage()
{
	puts(	"Tool to display an LDPC matrix.\n"
		"\n"
		"  -h   Display this help screen.\n"
		"  -K # Set K value."
	);
}

int main(int argc, char** argv)
{
	int K = 60;

	/* Read command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hK:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'K':
			K = atoi(optarg);
			break;
		case '?':
			exit(EXIT_FAILURE);
		};
	}

	/* Get parameters */
	parameters P = parameters_get(K);
	if (P.K == -1) {
		fprintf(stderr, "Error:  Invalid choice of K.\n");
		return 1;
	}

	/* Generate a suitable LDPC matrix */
	uint8_t* m = malloc(P.S * P.L * sizeof(uint8_t));
	m256v L = m256v_make(P.S, P.L, m);
	ldpc_generate_mat(&L, &P);

	/* Print on screen */
	for (int r = 0; r < P.S; ++r) {
		for (int c = 0; c < P.L; ++c) {
			switch (m256v_get_el(&L, r, c)) {
			case 0:		fputc('0', stdout);	break;
			case 1:		fputc('1', stdout);	break;
			default:
				fprintf(stderr, "Error: Non-binary matrix "
				  "entry seen, row=%d, col=%d\n", r, c);
				return 1;
			}
		}
		fputc('\n', stdout);
	}

	return 0;
}
