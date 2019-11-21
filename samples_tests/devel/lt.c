#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "lt.h"
#include "m256v.h"
#include "parameters.h"
#include "parse_esis.h"

static void usage()
{
	puts(	"Tool to display an LT matrix.\n"
		"\n"
		"  -h       Display this help screen.\n"
		"  -K #     Set K value.\n"
		"  -i #,... Symbol identifiers (ESIs)."
	);
}

int main(int argc, char** argv)
{
	int K = 60;
	int n_ISIs = 0;
	int n_max_ISIs = 32;
	uint32_t* ISIs = malloc(n_max_ISIs * sizeof(uint32_t));

	/* Read command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hK:i:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			break;
		case 'K':
			K = atoi(optarg);
			break;
		case 'i': {
			parse_esis(&n_ISIs, &n_max_ISIs, &ISIs, optarg);
			break;
		}
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

	/* Generate a suitable LT matrix */
	uint8_t* m = malloc(n_ISIs * P.L * sizeof(uint8_t));
	m256v M = m256v_make(n_ISIs, P.L, m);
	lt_generate_mat(&M, &P, n_ISIs, ISIs);

	/* Print on screen */
	for (int r = 0; r < M.n_row; ++r) {
		for (int c = 0; c < M.n_col; ++c) {
			switch (m256v_get_el(&M, r, c)) {
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
