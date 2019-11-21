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

static bool test_el_access()
{
	m2v_Def(A, a, 6, 5)
	Init_mat2(A,
		  1, 1, 0, 1, 1,
		  1, 0, 0, 1, 1,
		  1, 0, 1, 1, 1,
		  1, 0, 1, 0, 0,
		  0, 0, 0, 0, 1,
		  1, 0, 1, 0, 1, );

	/* Try out some reading patterns */
	const int diag[] =		{ 1, 0, 1, 0, 1 };
	const int first_row[] =		{ 1, 1, 0, 1, 1 };
	const int first_col[] =		{ 1, 1, 1, 1, 0, 1 };
	const int col3[] =		{ 1, 1, 1, 0, 0, 0 };
	const int dzigzag[] =		{ 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 };

#define check_pat(arr, ri, ci) \
	do { \
		for (int i = 0; i < array_size(arr); ++i) { \
			const int e = m2v_get_el(&A, (ri), (ci)); \
			if (arr[i] != e) { \
				fprintf(stderr, "Error in pattern `%s', " \
				  "index %d:  Got %d, expected %d.\n", \
				  #arr, i, (int)e, (int)arr[i]); \
				return false; \
			} \
		} \
	} while(0)
	check_pat(diag, i, i);
	check_pat(first_row, 0, i);
	check_pat(first_col, i, 0);
	check_pat(col3, i, 3);
	check_pat(dzigzag, (i + 1)/2, i/2);

	/* Modify and check what we read back */
	m2v_set_el(&A, 2, 1, 1);
	const int m_row2[] =	{ 1, 1, 1, 1, 1 };
	check_pat(m_row2, 2, i);

#undef check_pat
	return true;
}

static bool mptest_el_access2(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	const int niter = 1000;
	for (int N = 0; N < niter; ++N) {
		const int r = rand() % nrow;
		const int c = rand() % ncol;
		if (m256v_get_el(Ml, r, c) != m2v_get_el(Ms, r, c))
			return false;
	}
	return true;
}

/* This is a test of the _mat_pair functions themselves */
static bool mptest_testfuncs(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	const int r = rand() % nrow;
	const int c = rand() % ncol;

	for (int u = 0; u < 2; ++u) {
		m256v_set_el(Ml, r, c, u);
		for (int v = 0; v < 2; ++v) {
			m2v_set_el(Ms, r, c, v);
			if (check_mat_pair_equal(Ml, Ms) != (u == v)) {
				fprintf(stderr, "Error: check_mat_pair_equal() "
				  "failure detected.\n");
				return false;
			}
		}
	}
	return true;
}

static bool mptest_swap_rows(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < 10; ++i) {
		const int r1 = rand() % nrow;
		const int r2 = rand() % nrow;

		m256v_swap_rows(Ml, r1, r2);
		m2v_swap_rows(Ms, r1, r2);
	}
	return true;
}

static bool mptest_clear_row(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < 7; ++i) {
		const int r = rand() % nrow;

		m256v_clear_row(Ml, r);
		m2v_clear_row(Ms, r);
	}
	return true;
}

static bool mptest_multadd_row(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < 8; ++i) {
		const int r1 = rand() % nrow;
		const int rt = rand() % nrow;
		const int alpha = rand() & 1;

		m256v_multadd_row(Ml, r1, alpha, Ml, rt);
		m2v_multadd_row(Ms, r1, alpha, Ms, rt);
	}
	return true;
}

static bool mptest_multadd_row_from(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < 8; ++i) {
		const int r1 = rand() % nrow;
		const int rt = rand() % nrow;
		const int offs = rand() % ncol;
		const int alpha = rand() & 1;

		m256v_multadd_row_from(Ml, r1, offs, alpha, Ml, rt);
		m2v_multadd_row_from(Ms, r1, offs, alpha, Ms, rt);
	}
	return true;
}

static bool mptest_copy_row(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < 10; ++i) {
		const int r1 = rand() % nrow;
		const int r2 = rand() % nrow;

		m2v_copy_row(Ms, r1, Ms, r2);
		m256v_copy_row(Ml, r1, Ml, r2);
	}
	return true;
}

static bool mptest_row_iszero(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < nrow/2; ++i) {
		const int r = rand() % nrow;
		m2v_clear_row(Ms, r);
		m256v_clear_row(Ml, r);
	}

	for (int r = 0; r < nrow; ++r) {
		if (m2v_row_iszero(Ms, r) != m256v_row_iszero(Ml, r)) {
			return false;
		}
	}
	return true;
}

static bool mptest_swap_cols(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < ncol/3; ++i) {
		const int c1 = rand() % ncol;
		const int c2 = rand() % ncol;

		m2v_swap_cols(Ms, c1, c2);
		m256v_swap_cols(Ml, c1, c2);
	}
	return true;
}

static bool mptest_mult_col_from(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < ncol/3; ++i) {
		const int c = rand() % ncol;
		const int r = rand() % nrow;

		m2v_mult_col_from(Ms, c, r, 1);
		m256v_mult_col_from(Ml, c, r, 1);
	}

	for (int i = 0; i < ncol/3; ++i) {
		const int c = rand() % ncol;
		const int r = rand() % nrow;

		m2v_mult_col_from(Ms, c, r, 0);
		m256v_mult_col_from(Ml, c, r, 0);
	}
	return true;
}

static bool mptest_copy_col(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	for (int i = 0; i < ncol/3; ++i) {
		const int c1 = rand() % ncol;
		const int ct = rand() % ncol;

		m2v_copy_col(Ms, c1, Ms, ct);
		m256v_copy_col(Ml, c1, Ml, ct);
	}
	return true;
}

static bool mptest_iszero(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	int p[ncol];
	perm_rand(ncol, p);

	bool r = false;
	for (int i = 0; i < ncol; ++i) {
		m2v_mult_col_from(Ms, p[i], 0, 0);
		m256v_mult_col_from(Ml, p[i], 0, 0);

		if (r) {
			/* Can't become nonzero once it was zero */
			if (!m2v_iszero(Ms))
				return false;
		} else {
			if (m2v_iszero(Ms))
				r = true;
		}

		if (r != m256v_iszero(Ml))
			return false;
	}

	if (r == false) {
		/* Must be zero after all columns were cleared */
		return false;
	}
	return true;
}

static bool mptest_clear(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	m2v_clear(Ms);
	m256v_clear(Ml);
	return true;
}

static bool test_copy()
{
	for (int n_row = 3; n_row < 50; n_row = n_row * 5/2) {
		for (int n_col = 3; n_col < 130; n_col *= n_col * 5/2) {
			/* Create stimulus */
			m256v Msl;
			m2v Mss;
			get_mat_pair(n_row, n_col, &Msl, &Mss);

			m256v Mtl;
			m2v Mts;
			get_mat_pair(n_row, n_col, &Mtl, &Mts);
			clobber_m2v(&Mts);

			/* Copy */
			m2v_copy(&Mss, &Mts);
			m256v_copy(&Msl, &Mtl);

			/* Check & Free */
			bool succ = check_mat_pair_equal(&Mtl, &Mts);
			free_mat_pair_mem(&Mtl, &Mts);
			free_mat_pair_mem(&Msl, &Mss);

			if (!succ)
				return false;
		}
	}
	return true;
}

static bool mptest_permute_rows(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	int p[nrow];
	perm_rand(nrow, p);
	m2v_permute_rows(Ms, p);
	m256v_permute_rows(Ml, p);
	return true;
}

static bool mptest_permute_cols(int nrow, int ncol, m256v* Ml, m2v* Ms)
{
	int p[ncol];
	perm_rand(ncol, p);
	m2v_permute_cols(Ms, p);
	m256v_permute_cols(Ml, p);
	return true;
}

static bool test_add()
{
	for (int n_row = 10; n_row < 50; n_row = n_row * 17/10) {
		for (int n_col = 10; n_col < 84; n_col *= n_col * 17/10) {
			/* Create stimulus */
			m256v Al;
			m2v As;
			get_mat_pair(n_row, n_col, &Al, &As);

			m256v Bl;
			m2v Bs;
			get_mat_pair(n_row, n_col, &Bl, &Bs);

			m256v Cl;
			m2v Cs;
			get_mat_pair(n_row, n_col, &Cl, &Cs);
			clobber_m2v(&Cs);

			/* Perform add */
			m2v_add(&As, &Bs, &Cs);
			m256v_add(&Al, &Bl, &Cl);

			/* Check & Free */
			bool succ = check_mat_pair_equal(&Cl, &Cs);
			free_mat_pair_mem(&Cl, &Cs);
			free_mat_pair_mem(&Bl, &Bs);
			free_mat_pair_mem(&Al, &As);

			if (!succ)
				return false;
		}
	}
	return true;
}

static bool test_add_inplace()
{
	for (int n_row = 7; n_row < 50; n_row = n_row * 19/10) {
		for (int n_col = 7; n_col < 50; n_col *= n_col * 19/10) {
			/* Create stimulus */
			m256v Al;
			m2v As;
			get_mat_pair(n_row, n_col, &Al, &As);

			m256v Bl;
			m2v Bs;
			get_mat_pair(n_row, n_col, &Bl, &Bs);

			/* Perform add */
			m2v_add_inplace(&As, &Bs);
			m256v_add_inplace(&Al, &Bl);

			/* Check & Free */
			bool succ = check_mat_pair_equal(&Bl, &Bs);
			free_mat_pair_mem(&Bl, &Bs);
			free_mat_pair_mem(&Al, &As);

			if (!succ)
				return false;
		}
	}
	return true;
}

static bool test_mul()
{
	const int inner_sz = 99;

	for (int n_row = 10; n_row < 50; n_row = n_row * 17/10) {
		for (int n_col = 10; n_col < 84; n_col *= n_col * 17/10) {
			/* Create stimulus */
			m256v Al;
			m2v As;
			get_mat_pair(n_row, inner_sz, &Al, &As);

			m256v Bl;
			m2v Bs;
			get_mat_pair(inner_sz, n_col, &Bl, &Bs);

			m256v Cl;
			m2v Cs;
			get_mat_pair(n_row, n_col, &Cl, &Cs);
			clobber_m2v(&Cs);

			/* Perform add */
			m2v_mul(&As, &Bs, &Cs);
			m256v_mul(&Al, &Bl, &Cl);

			/* Check & Free */
			bool succ = check_mat_pair_equal(&Cl, &Cs);
			free_mat_pair_mem(&Cl, &Cs);
			free_mat_pair_mem(&Bl, &Bs);
			free_mat_pair_mem(&Al, &As);

			if (!succ)
				return false;
		}
	}
	return true;
}

static void usage()
{
	puts(	"Tests for basic m2v functionality.\n"
		"\n"
		"This excludes tests of LU related functions, for which\n"
		"there is a separate utility.\n"
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

	// Tests for the elementary ops on submatrices
	RUN_TEST(test_el_access());
	RUN_TEST(run_mat_pair_test(mptest_el_access2));
	RUN_TEST(run_mat_pair_test(mptest_testfuncs));
	RUN_TEST(run_mat_pair_test(mptest_swap_rows));
	RUN_TEST(run_mat_pair_test(mptest_clear_row));
	RUN_TEST(run_mat_pair_test(mptest_multadd_row));
	RUN_TEST(run_mat_pair_test(mptest_multadd_row_from));
	RUN_TEST(run_mat_pair_test(mptest_copy_row));
	RUN_TEST(run_mat_pair_test(mptest_row_iszero));
	RUN_TEST(run_mat_pair_test(mptest_swap_cols));
	RUN_TEST(run_mat_pair_test(mptest_mult_col_from));
	RUN_TEST(run_mat_pair_test(mptest_copy_col));

	// Tests for the entire matrix
	RUN_TEST(run_mat_pair_test(mptest_iszero));
	RUN_TEST(run_mat_pair_test(mptest_clear));
	RUN_TEST(test_copy());
	RUN_TEST(run_mat_pair_test(mptest_permute_rows));
	RUN_TEST(run_mat_pair_test(mptest_permute_cols));
	RUN_TEST(test_add());
	RUN_TEST(test_add_inplace());
	RUN_TEST(test_mul());
#undef RUN_TEST

	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
