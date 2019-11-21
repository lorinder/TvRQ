#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "rq_matrix.h"
#include "m256v.h"
#include "parameters.h"

#include "parse_esis.h"

static void usage()
{
	puts(	"Tool to display an entire RQ matrix.\n"
		"\n"
		"  -h       Display this help screen.\n"
		"  -K #     Set K value.\n"
		"  -i #,... Symbol identifiers (ESIs)."
	);
}


int main(int argc, char** argv)
{
	int K = 60;
	int n_ESIs = 0;
	int n_max_ESIs = 32;
	uint32_t* ESIs = malloc(n_max_ESIs * sizeof(uint32_t));

	/* Read command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hK:i:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'K':
			K = atoi(optarg);
			break;
		case 'i': {
			parse_esis(&n_ESIs, &n_max_ESIs, &ESIs, optarg);
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

	/* Generate a suitable rq_matrix */
	int n_rows, n_cols;
	rq_matrix_get_dim(&P, n_ESIs, &n_rows, &n_cols);
	uint8_t* m = malloc(sizeof(uint8_t) * n_rows * n_cols);
	m256v M = m256v_make(n_rows, n_cols, m);
	rq_matrix_generate(&M, &P, n_ESIs, ESIs);

	/* Print on screen */
	for (int r = 0; r < M.n_row; ++r) {
		for (int c = 0; c < M.n_col; ++c) {
			switch (m256v_get_el(&M, r, c)) {
			case 0:		fputc('0', stdout);	break;
			case 1:		fputc('1', stdout);	break;
			default:	fputc('.', stdout);	break;
			}
		}
		fputc('\n', stdout);
	}

	return 0;
}
