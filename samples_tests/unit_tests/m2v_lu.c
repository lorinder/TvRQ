#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "m256v.h"
#include "m2v.h"
#include "perm.h"

#include "utils.h"
#include "m2v_m256v_mat_pair.h"

static bool stest_m2v_lu(int nrow, int ncol)
{
	m256v Ml;
	m2v Ms;
	get_mat_pair(nrow, ncol, &Ml, &Ms);

	/* Execute on GF2 matrix */
	int rps[nrow], cps[ncol];
	const int rs = m2v_LU_decomp_inplace(&Ms, rps, cps);

	/* Execute on GF256 matrix */
	int rpl[nrow], cpl[ncol];
	const int rl = m256v_LU_decomp_inplace(&Ml, rpl, cpl);

	/* Compare results */
	bool success = true;
	if (!check_mat_pair_equal(&Ml, &Ms)) {
		printf("Failure detected:  LU matrices are not the same.\n");
		success = false;
	}
	for (int i = 0; i < nrow; ++i) {
		if (rpl[i] != rps[i]) {
			printf("Failure detected:  Row permutation is not the same.\n");
			success = false;
			break;
		}
	}
	for (int i = 0; i < ncol; ++i) {
		if (cpl[i] != cps[i]) {
			printf("Failure detected:  Col permutation is not the same.\n");
			success = false;
			break;
		}
	}
	if (rl != rs) {
		printf("Failure detected:  rank is not the same.\n");
		success = false;
	}

	/* Free mem, finish up */
	free_mat_pair_mem(&Ml, &Ms);
	return success;
}

bool test_m2v_lu()
{
	/* Aiming for about 20 iterations with the largest matrix being
	 * tested.  The smaller matrices have a correspondingly scaled
	 * up number of iterations.
	 */
	const int nit_scale = 10000 * 20;

	for (int nrow = 3; nrow < 100; nrow = nrow * 283 / 200) {
		for (int ncol = 3; ncol < 100; ncol = ncol * 283 / 200) {
			const int nit = nit_scale / (nrow * ncol);
			for (int i = 0; i < nit; ++i) {
				if (!stest_m2v_lu(nrow, ncol)) {
					printf("Failure seen in iteration %d.\n", i);
					return false;
				}
			}
		}
	}
	return true;
}

bool stest_m2v_mult(int nrow, int ncol)
{
	/* Generate inputs and prepare output mat */
	m256v Ml;
	m2v Ms;
	get_mat_pair(nrow, ncol, &Ml, &Ms);

	m256v Xl;
	m2v Xs;
	get_mat_pair(ncol, 16, &Xl, &Xs);

	m256v Yl;
	m2v Ys;
	get_mat_pair(nrow, 16, &Yl, &Ys);
	clobber_m2v(&Ys);

	/* LU decomp */
	int rps[nrow], cps[ncol];
	m2v_LU_decomp_inplace(&Ms, rps, cps);

	int rpl[nrow], cpl[ncol];
	m256v_LU_decomp_inplace(&Ml, rpl, cpl);

	/* Generate output */
	m2v_LU_mult(&Ms, rps, cps, &Xs, &Ys);
	m256v_LU_mult(&Ml, rpl, cpl, &Xl, &Yl);

	/* Check results
	 *
	 * We could potentially also check all the intermediate results,
	 * such as LU decompositions, etc.  But since we already did
	 * that in the previous check, there is no real value in doing
	 * it again here.  In fact, it's better not to repeat the tests,
	 * for cases e.g. where one of the LU routines were changed to
	 * behave a bit differently, though still be correct.  Then we
	 * could do the end to end tests, but the LU matrices themselves
	 * may not match.
	 */
	bool success = true;
	if (!check_mat_pair_equal(&Yl, &Ys)) {
		printf("Failure detected:  LU multiplication results differ.\n");
		success = false;
	}

	/* Free mem, finish up */
	free_mat_pair_mem(&Yl, &Ys);
	free_mat_pair_mem(&Xl, &Xs);
	free_mat_pair_mem(&Ml, &Ms);
	return success;
}

bool test_m2v_mult()
{
	for (int nrow = 3; nrow < 100; nrow = nrow * 283 / 200) {
		for (int ncol = 3; ncol < 100; ncol = ncol * 283 / 200) {
			for (int i = 0; i < 10; ++i) {
				if (!stest_m2v_mult(nrow, ncol)) {
					printf("Failure seen in iteration %d.\n", i);
					return false;
				}
			}
		}
	}
	return true;
}

bool stest_m2v_mult_inplace(int nrow, int ncol)
{
	/* Generate inputs and prepare output mat */
	m256v Ml;
	m2v Ms;
	get_mat_pair(nrow, ncol, &Ml, &Ms);

	m256v Yl;
	m2v Ys;
	get_mat_pair(nrow > ncol ? nrow : ncol, 16, &Yl, &Ys);

	/* LU decomp */
	int rps[nrow], cps[ncol];
	m2v_LU_decomp_inplace(&Ms, rps, cps);
	int inv_rps[nrow];
	perm_invert(nrow, rps, inv_rps);

	int rpl[nrow], cpl[ncol];
	m256v_LU_decomp_inplace(&Ml, rpl, cpl);
	int inv_rpl[nrow];
	perm_invert(nrow, rpl, inv_rpl);

	/* Generate output */
	m2v_LU_mult_inplace(&Ms, inv_rps, cps, &Ys);
	Ys.n_row = nrow;
	m256v_LU_mult_inplace(&Ml, inv_rpl, cpl, &Yl);
	Yl.n_row = nrow;

	/* Check results
	 *
	 * We could potentially also check all the intermediate results,
	 * such as LU decompositions, etc.  But since we already did
	 * that in the previous check, there is no real value in doing
	 * it again here.  In fact, it's better not to repeat the tests,
	 * for cases e.g. where one of the LU routines were changed to
	 * behave a bit differently, though still be correct.  Then we
	 * could do the end to end tests, but the LU matrices themselves
	 * may not match.
	 */
	bool success = true;
	if (!check_mat_pair_equal(&Yl, &Ys)) {
		printf("Failure detected:  LU multiplication results differ.\n");
		success = false;
	}

	/* Free mem, finish up */
	free_mat_pair_mem(&Yl, &Ys);
	free_mat_pair_mem(&Ml, &Ms);
	return success;
}

bool test_m2v_mult_inplace()
{
	for (int nrow = 3; nrow < 100; nrow = nrow * 283 / 200) {
		for (int ncol = 3; ncol < 100; ncol = ncol * 283 / 200) {
			for (int i = 0; i < 10; ++i) {
				if (!stest_m2v_mult_inplace(nrow, ncol)) {
					printf("Failure seen in iteration %d.\n", i);
					return false;
				}
			}
		}
	}
	return true;
}

bool stest_m2v_invmult_inplace(int nrow, int ncol)
{
	/* Generate inputs and prepare output mat */
	m256v Ml;
	m2v Ms;
	get_mat_pair(nrow, ncol, &Ml, &Ms);

	m256v Yl;
	m2v Ys;
	get_mat_pair(nrow > ncol ? nrow : ncol, 16, &Yl, &Ys);

	/* LU decomp */
	int rps[nrow], cps[ncol];
	const int rs = m2v_LU_decomp_inplace(&Ms, rps, cps);
	int inv_cps[ncol];
	perm_invert(ncol, cps, inv_cps);

	int rpl[nrow], cpl[ncol];
	const int rl = m256v_LU_decomp_inplace(&Ml, rpl, cpl);
	int inv_cpl[ncol];
	perm_invert(ncol, cpl, inv_cpl);

	/* Generate output */
	m2v_LU_invmult_inplace(&Ms, rs, rps, inv_cps, &Ys);
	Ys.n_row = ncol;
	m256v_LU_invmult_inplace(&Ml, rl, rpl, inv_cpl, &Yl);
	Yl.n_row = ncol;

	/* Check results
	 *
	 * We could potentially also check all the intermediate results,
	 * such as LU decompositions, etc.  But since we already did
	 * that in the previous check, there is no real value in doing
	 * it again here.  In fact, it's better not to repeat the tests,
	 * for cases e.g. where one of the LU routines were changed to
	 * behave a bit differently, though still be correct.  Then we
	 * could do the end to end tests, but the LU matrices themselves
	 * may not match.
	 */
	bool success = true;
	if (!check_mat_pair_equal(&Yl, &Ys)) {
		printf("Failure detected:  LU multiplication results differ.\n");
		success = false;
	}

	/* Free mem, finish up */
	free_mat_pair_mem(&Yl, &Ys);
	free_mat_pair_mem(&Ml, &Ms);
	return success;
}

bool test_m2v_invmult_inplace()
{
	for (int nrow = 3; nrow < 100; nrow = nrow * 283 / 200) {
		for (int ncol = 3; ncol < 100; ncol = ncol * 283 / 200) {
			for (int i = 0; i < 10; ++i) {
				if (!stest_m2v_invmult_inplace(nrow, ncol)) {
					printf("Failure seen in iteration %d.\n", i);
					return false;
				}
			}
		}
	}
	return true;
}

static void usage()
{
	puts(	"Tests for m2v LU functionality.\n"
		"\n"
		"  -h   Display this help screen.\n"
		);
}

int main(int argc, char** argv)
{
	/* Read cmdline args */
	int c;
	while ((c = getopt(argc, argv, "h")) != -1) {
		switch(c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case '?':
			exit(EXIT_FAILURE);
		};
	}

	int nfail = 0;
#define RUN_TEST(x) \
	do { \
		printf("Running: " #x "\n"); \
		if (!x) { \
			printf("--> FAIL (test " #x ")\n"); \
			++nfail; \
		} else { \
			printf("--> pass\n"); \
		} \
	} while(0)

	// LU routines test
	RUN_TEST(test_m2v_lu());
	RUN_TEST(test_m2v_mult());
	RUN_TEST(test_m2v_mult_inplace());
	RUN_TEST(test_m2v_invmult_inplace());
#undef RUN_TEST

	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
