#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include "gf256.h"
#include "m256v.h"
#include "perm.h"

#include "utils.h"

static void rand_mat(m256v* M, uint8_t mask)
{
	for (int i = 0; i < M->n_row; ++i) {
		for (int j = 0; j < M->n_col; ++j) {
			m256v_set_el(M, i, j, rand() & mask);
		}
	}
}

static void get_lu_split_testcase(
		m256v* A,
		m256v* X,
		m256v* Y,
		int field)
{
	const int n_row = A->n_row, n_col = A->n_col;
	assert(n_row >= n_col);
	const uint8_t mask = get_mask(field);

	/* Find a full rank test case */
	int rank;
	do {
		/* Randomize A */
		rand_mat(A, mask);

		/* LU decompose & check rank */
		m256v_Def(LU, lu, n_row, n_col)
		m256v_copy(A, &LU);
		int rp[n_row], cp[n_col];
		rank = m256v_LU_decomp_inplace(&LU, rp, cp);
	} while (rank < n_col);

	/* Pick a random X, compute Y */
	rand_mat(X, mask);
	m256v_mul(A, X, Y);
}

static bool stest_lusplit(int n_row,
		int n_col,
		int field,
		int n_top_rows,
		bool verbose)
{
	const int dlen = 4;
	m256v_Def(A, a, n_row, n_col)
	m256v_Def(X, x, n_col, dlen)
	m256v_Def(Y, y, n_row, dlen)

	/* Create the test case */
	get_lu_split_testcase(&A, &X, &Y, field);
	if (verbose) {
		puts("A:");	print_mat(stdout, " ", &A);
		puts("X:");	print_mat(stdout, " ", &X);
		puts("Y:");	print_mat(stdout, " ", &Y);
	}

	/* LU decompose the top part */
	m256v A_top = m256v_get_subview(&A,
				0, 0,
				n_top_rows, n_col);
	m256v_Def(LU_top, lu_top, n_top_rows, n_col)
	m256v_copy(&A_top, &LU_top);
	int rp_top[n_top_rows], cp_top[n_col];
	int rank_top = m256v_LU_decomp_inplace(&LU_top, rp_top, cp_top);
	if (verbose) {
		printf("rank top: %d\n", rank_top);
		printf("rp_top: ");
		print_int_arr(stdout, n_top_rows, rp_top);
		printf("cp_top: ");
		print_int_arr(stdout, n_col, cp_top);
		puts("LU_top:");
		print_mat(stdout, " ", &LU_top);
	}

	/* Prepare work & result memory */
	m256v_Def(Y2, y2, n_row, dlen)
	m256v_copy(&Y, &Y2);
	m256v X2 = m256v_get_subview(&Y2, 0, 0, n_col, Y2.n_col);

	if (rank_top == n_col) {
		/* If the rank of the top matrix is n_col, we don't need
		 * to solve the lower matrix, so we can skip this
		 * entirely.
		 *
		 * We're not applying the inverse multiplication here
		 * because it will be performed later.
		 */
		m256v_LU_invmult_inplace(&LU_top, rank_top, rp_top, NULL, &Y2);
	} else {
		/* Top part is not full rank, thus we need to solve the
		 * bottom part as well.
		 */

		/* Split top part into LU proper and backsubstitution part */
		m256v LU_tops = m256v_get_subview(&LU_top,
					0, 0,
					n_top_rows, rank_top);
		m256v B_tops = m256v_get_subview(&LU_top,
					0, rank_top,
					n_top_rows, n_col - rank_top);

		/* Compute backsubstitution matrix */
		m256v_U_invmult_inplace(&LU_tops, rank_top, &B_tops);
		if (verbose) {
			puts("top Backsubstitution matrix:");
			print_mat(stdout, " ", &B_tops);
		}

		/* Compute corresponding right hand side updates */
		m256v Y2_top = m256v_get_subview(&Y2,
					0, 0,
					n_top_rows, dlen);
		m256v_LU_invmult_inplace(&LU_tops, rank_top,
					rp_top, NULL, &Y2_top);
		if (verbose) {
			puts("Y2_top, top right hand side before backsubst:");
			print_mat(stdout, " ", &Y2_top);
		}


		/* Compute bottom matrix */
		const int n_bot_rows = n_row - n_top_rows;
		m256v A_bot = m256v_get_subview(&A,
					n_top_rows, 0,
					n_bot_rows, n_col);
		m256v_permute_cols(&A_bot, cp_top);
		if (verbose) {
			puts("A_bot, cols permuted:");
			print_mat(stdout, " ", &A_bot);
		}

		/* Ar_bot, bottom matrix with elimination executed */
		const int n_bot_col = n_col - rank_top;
		m256v Ar_bot = m256v_get_subview(&A_bot,
					0, rank_top,
					n_bot_rows, n_bot_col);
		m256v Y2_bot = m256v_get_subview(&Y2,
					n_top_rows, 0,
					n_bot_rows, dlen);
		for (int i = 0; i < n_bot_rows; ++i) {
			for (int j = 0; j < rank_top; ++j) {
				const uint8_t v = m256v_get_el(&A_bot, i, j);
				m256v_multadd_row(&B_tops, j, v, &Ar_bot, i);
				m256v_multadd_row(&Y2_top, j, v, &Y2_bot, i);
			}
		}
		if (verbose) {
			puts("Ar_bot:");
			print_mat(stdout, " ", &Ar_bot);
			puts("Y2_bot before solving:");
			print_mat(stdout, " ", &Y2_bot);
		}

		/* Solve for the bottom matrix */
		int rp_bot[n_bot_rows], cp_bot[n_bot_col];
		int rank_bot = m256v_LU_decomp_inplace(&Ar_bot, rp_bot, cp_bot);
		if (rank_bot != n_bot_col) {
			fprintf(stderr, "Error:%s:%d:  Matrix not full rank.\n",
				__FILE__, __LINE__);
			return false;
		}
		m256v_LU_invmult_inplace(&Ar_bot, rank_bot, rp_bot, NULL, &Y2_bot);
		if (verbose) {
			puts("Y2_bot after solving:");
			print_mat(stdout, " ", &Y2_bot);
		}

		/* Backsubstitute */
		for (int i = 0; i < rank_top; ++i) {
			for (int j = 0 ; j < n_col - rank_top; ++j) {
				const uint8_t v = m256v_get_el(&B_tops, i, j);
				m256v_multadd_row(&Y2_bot, j, v, &Y2_top, i);
			}
		}
		if (verbose) {
			puts("Y2 after backsubstitution, before reordering.");
			print_mat(stdout, " ", &Y2);
		}

		/* Move second solution to the right place */
		if (rank_top < n_top_rows) {
			for (int i = 0; i < Y2_bot.n_row; ++i) {
				m256v_copy_row(&Y2_bot, i, &Y2, rank_top + i);
			}
		}
	}

	/* Reorder variables */
	int inv_cp[n_col];
	perm_invert(n_col, cp_top, inv_cp);
	m256v_permute_rows(&X2, inv_cp);

	/* Display result */
	if (verbose) {
		printf("reconstructed X:\n");
		print_mat(stdout, " ", &X2);
	}

	/* Check result */
	m256v_add_inplace(&X, &X2);
	if (m256v_iszero(&X2))
		return true;
	return false;
}

static bool test_lusplit(bool verbose,
		int niter,
		int n_row,
		int n_col,
		int n_bot_row,
		int field)
{
	for (int i = 0; i < niter; ++i) {
		const bool
		  succ = stest_lusplit(
				n_row,
				n_col,
				field,
				n_row - n_bot_row,
				verbose);
		if (!succ) {
			fprintf(stderr, "Failure for i=%d\n", i);
			return false;
		}
	}
	return true;
}

static int test_lusplit_with_feedback(
		bool verbose,
		int niter,
		int n_row,
		int n_col,
		int n_bot_row,
		int field)
{
	printf("Running splitsolve test (verbose=%d, "
		"niter=%d, n_row=%d, n_col=%d, "
		"n_bot_row=%d, field=%d)\n",
			(int)verbose,
			niter,
			n_row,
			n_col,
			n_bot_row,
			field);
	bool x = test_lusplit(verbose,
			niter,
			n_row,
			n_col,
			n_bot_row,
			field);
	if (!x) {
		printf("--> FAIL\n");
		return 1;
	} else {
		printf("--> pass\n");
		return 0;
	}
}

static void usage()
{
	puts(	"Test for solving linear system in two parts.\n"
		"\n"
		"  -h     Display this help screen\n"
		"  -s #   Set RNG seed\n"
		"  -n #   Number of iterations\n"
		"  -v     Verbose: print all results, even intermediate ones\n"
		"\n"
		"Options can steer the kind of matrix tested.  If none of the\n"
		"below options are set, a grid run over different shapes\n"
		"is performed.\n"
		"  -r #   Number of rows in the system\n"
		"  -c #   Number of columns in the system (must be <= # rows)\n"
		"  -b #   Number of rows in the bottom matrix\n"
		"  -f #   The field to use, 2 or 256"
	    );
}

int main(int argc, char** argv)
{
	unsigned seed = time(0);
	bool gridrun = true;

	bool verbose = false;
	int niter = 5000;
	int n_row = 4;
	int n_col = 4;
	int n_bot_row = 2;
	int field = 2;

	/* Read command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hs:n:vr:c:b:f:")) != -1) {
		switch(c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 's':
			seed = atoi(optarg);
			break;
		case 'n':
			niter = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case 'r':
			n_row = atoi(optarg);
			gridrun = false;
			break;
		case 'c':
			n_col = atoi(optarg);
			gridrun = false;
			break;
		case 'b':
			n_bot_row = atoi(optarg);
			gridrun = false;
			break;
		case 'f':
			field = atoi(optarg);
			gridrun = false;
			break;
		case '?':
			exit(EXIT_FAILURE);
		};
	}

	/* Seed the RNG */
	srand(seed);
	printf("RNG seed used: %u\n", seed);

	if (!gridrun) {
		/* Check the args */
#define ERR_IF(cond, msg) \
		do { \
			if (cond) { \
				fprintf(stderr, "Error: %s\n", (msg)); \
				return EXIT_FAILURE; \
			} \
		} while(0)
		ERR_IF(n_col < 0, "Negative number of columns selected");
		ERR_IF(n_row < n_col, "Need at least as many columns as rows");
		ERR_IF(n_bot_row >= n_row, "The number of bottom rows needs to be less than"
				" the number of rows");
		ERR_IF(field != 2 && field != 256, "The field must be either GF(2) or GF(256)");
#undef ERR_IF
	}

	/* Run the various tests */
	int nfail = 0;
	if (!gridrun) {
		nfail += test_lusplit_with_feedback(verbose,
					niter,
					n_row,
					n_col,
					n_bot_row,
					field);
	} else {
		const int fields[] = { 2, 256 };
		for (int n_row = 2; n_row < 7; ++n_row) {
			for (int n_col = 1; n_col <= n_row; ++n_col) {
				for (int n_bot_row = 0;
					n_bot_row < 4 && n_bot_row < n_row;
					++n_bot_row)
				{
					for (int fi = 0;
						fi < array_size(fields);
						++fi)
					{
						const int field = fields[fi];
						nfail
						  += test_lusplit_with_feedback(
								verbose,
								niter,
								n_row,
								n_col,
								n_bot_row,
								field);
					}
				}
			}
		}
	}

	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
