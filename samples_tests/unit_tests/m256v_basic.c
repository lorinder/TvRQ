#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "gf256.h"
#include "m256v.h"
#include "perm.h"

#include "utils.h"

#define fadd	gf256_add
#define fmul	gf256_mul

static bool test_get_el_offs()
{
	/* Check row major order */
	const int nrow = 7, ncol = 11;
	m256v_Def(A, mem, nrow, ncol)
	size_t offs = 0;
	for (int r = 0; r < nrow; ++r) {
		for (int c = 0; c < ncol; ++c) {
			size_t result = m256v_get_el_offs(&A, r, c);
			if (result != offs) {
				fprintf(stderr,
				  "  m256v_get_el_offs(A, %d, %d) = %zu\n",
				  r, c, result);
				fprintf(stderr,
				  "  would have expected %zu.  A is %d rows "
				  "x %d cols\n", offs, nrow, ncol);
				return false;
			}
			++offs;
		}
	}

	return true;
}

static bool test_el_access()
{
	Def_mat256_init(A, a, 6, 5,
		  3, 1, 4, 1, 5,
		  9, 2, 6, 5, 3,
		  5, 8, 9, 7, 9,
		  3, 2, 3, 8, 4,
		  6, 2, 6, 4, 3,
		  3, 8, 3, 2, 7, )
	assert(array_size(a) == 6 * 5);

	/* Try out some reading patterns */
	const uint8_t diag[] =		{ 3, 2, 9, 8, 3 };
	const uint8_t first_row[] =	{ 3, 1, 4, 1, 5 };
	const uint8_t first_col[] =	{ 3, 9, 5, 3, 6, 3 };
	const uint8_t col3[] =		{ 1, 5, 7, 8, 4, 2 };
	const uint8_t dzigzag[] =	{ 3, 9, 2, 8, 9, 3, 8, 4, 3, 7 };

#define check_pat(arr, ri, ci) \
	do { \
		for (int i = 0; i < array_size(arr); ++i) { \
			const uint8_t e = m256v_get_el(&A, (ri), (ci)); \
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
	m256v_set_el(&A, 2, 1, 123);
	const uint8_t m_row2[] =	{ 5, 123, 9, 7, 9 };
	check_pat(m_row2, 2, i);

#undef check_pat
	return true;
}

static bool test_swap_rows()
{
	Def_mat256_init(A, m, 3, 2,
		  1, 2,
	          3, 4,
		  5, 6)

	m256v_swap_rows(&A, 1, 2);
	const uint8_t sw[] = { 1, 2,
	                       5, 6,
			       3, 4 };
	if (memcmp(m, sw, sizeof(sw)) != 0) {
		fprintf(stderr,
		  "%s:%d:  Different from hard coded row swap result.\n",
		  __FILE__, __LINE__);
		return false;
	}

	return true;
}

static bool test_clear_row()
{
	Def_mat256_init(A, a, 3, 2,
		         1, 2,
	                 3, 4,
			 5, 6 )
	m256v_clear_row(&A, 1);
	const uint8_t sw[] = { 1, 2,
	                       0, 0,
			       5, 6 };
	if (memcmp(a, sw, sizeof(sw)) != 0) {
		fprintf(stderr,
		  "%s:%d:  Different from hard coded row clear result.\n",
		  __FILE__, __LINE__);
		return false;
	}
	return true;
}

static bool test_multadd_row()
{
	Def_mat256_init(A, m, 3, 2,
		         110, 120,
	                 130, 140,
			 150, 160 )

	m256v_multadd_row(&A, 1, 13, &A, 2);
	const uint8_t sw[] = { 110, 120,
	                       130, 140,
			        66,  50 };
	if (memcmp(m, sw, sizeof(sw)) != 0) {
		fprintf(stderr,
		  "%s:%d:  Different from hard coded mult-add row result.\n",
		  __FILE__, __LINE__);
		return false;
	}
	return true;
}

static bool test_iszero()
{
	// Hard coded zero example
	Def_mat256_init(A, a, 3, 3,
			0, 0, 0,
			0, 0, 0,
			0, 0, 0 )
	if (!m256v_iszero(&A))
		return false;

	// Hard coded nonzero example
	m256v_set_el(&A, 1, 1, 0x1);
	if (m256v_iszero(&A))
		return false;

	return true;
}

static bool test_clear()
{
	const int nrow[] = { 1, 2, 3, 5, 9, 10 };
	const int ncol[] = { 1, 2, 3, 7, 8, 10 };
	for (int ir = 0; ir < array_size(nrow); ++ir) {
		const int nr = nrow[ir];
		for (int ic = 0; ic < array_size(ncol); ++ic) {
			const int nc = ncol[ic];

			/* Create a random matrix, clear it, and verify
			 * it's cleared
			 */
			Def_mat256_rand(A, a, nr, nc, 0xff)
			m256v_clear(&A);
			if (!m256v_iszero(&A))
				return false;
		}
	}

	return true;
}

static bool test_copy()
{
	const int nrow[] = { 1, 2, 3, 5, 9, 10 };
	const int ncol[] = { 1, 2, 3, 7, 8, 10 };
	for (int ir = 0; ir < array_size(nrow); ++ir) {
		const int nr = nrow[ir];
		for (int ic = 0; ic < array_size(ncol); ++ic) {
			const int nc = ncol[ic];

			Def_mat256_rand(A, a, nr, nc, 0xff)
			m256v_Def(B, b, nr, nc);
			m256v_copy(&A, &B);

			for (int r = 0; r < nr; ++r) {
				for (int c = 0; c < nc; ++c) {
					if (m256v_get_el(&A, r, c)
					   != m256v_get_el(&B, r, c))
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

static bool test_permute_rows()
{
	const int rowscols[] = { 1, 2, 3, 5, 8, 10, 15 };
	for (int i = 0; i < array_size(rowscols); ++i) {
		const int rc = rowscols[i];

		/* Generate a random permutation */
		int p[rc];
		perm_rand(rc, p);

		/* Generate identity matrix */
		m256v_Def(A, a, rc, rc);
		for (int r = 0; r < rc; ++r) {
			for (int c = 0; c < rc; ++c) {
				m256v_set_el(&A, r, c, r==c);
			}
		}

		/* Apply perm */
		m256v_permute_rows(&A, p);

		/* Check output is as expected */
		for (int r = 0; r < rc; ++r) {
			int j;
			for (j = 0; j < rc; ++j) {
				if (m256v_get_el(&A, r, j))
					break;
			}

			if (j != p[r])
				return false;
		}
	}

	return true;
}

static bool test_permute_cols()
{
	const int rowscols[] = { 1, 2, 3, 5, 8, 10, 15 };
	for (int i = 0; i < array_size(rowscols); ++i) {
		const int rc = rowscols[i];

		/* Generate a random permutation */
		int p[rc];
		perm_rand(rc, p);

		/* Generate identity matrix */
		m256v_Def(A, a, rc, rc);
		for (int r = 0; r < rc; ++r) {
			for (int c = 0; c < rc; ++c) {
				m256v_set_el(&A, r, c, r==c);
			}
		}

		/* Apply perm */
		m256v_permute_cols(&A, p);

		/* Check output is as expected */
		for (int c = 0; c < rc; ++c) {
			int j;
			for (j = 0; j < rc; ++j) {
				if (m256v_get_el(&A, j, c))
					break;
			}

			if (j != p[c])
				return false;
		}
	}

	return true;
}

static bool test_add()
{
	/* Hard coded addition */
	{
		Def_mat256_init(A, a, 3, 2,  38, 55, 89, 12, 61, 94 )
		Def_mat256_init(B, b, 3, 2,  65, 46, 57, 86, 62,  9 )
		m256v_Def(S, s, 3, 2)
		m256v_add(&A, &B, &S);

		const uint8_t t[] = { 103, 25, 96, 90,  3, 87 };
		if (memcmp(s, t, sizeof(t)) != 0) {
			fprintf(stderr,
			  "%s:%d:  Different from hard coded sum result.\n",
			  __FILE__, __LINE__);
			return false;
		}

	}
	return true;
}

static bool test_add_inplace()
{
	/* Hard coded addition */
	{
		Def_mat256_init(A, a, 3, 2,  38, 55, 89, 12, 61, 94 )
		Def_mat256_init(B, b, 3, 2,  65, 46, 57, 86, 62,  9 )
		m256v_add_inplace(&A, &B);

		const uint8_t t[] = { 103, 25, 96, 90,  3, 87 };
		if (memcmp(b, t, sizeof(t)) != 0) {
			fprintf(stderr,
			  "%s:%d:  Different from hard coded sum result.\n",
			  __FILE__, __LINE__);
			return false;
		}

	}
	return true;
}

static bool test_mul0()
{
	/* Hard coded multiply */
	{
		Def_mat256_init(A, a, 2, 3,
		                 38, 55, 89,
		                 12, 61, 94 )
		Def_mat256_init(B, b, 3, 2,
		                 65, 46,
		                 57, 86,
				 62,  9 )
		m256v_Def(S, s, 2, 2);
		m256v_mul(&A, &B, &S);
#define s3(a, b, c)	fadd((a), fadd((b), (c)))
#define x(a, b)		fmul((a), (b))
		const uint8_t t[] = { s3(x(38,65), x(55,57), x(89,62)),
				      s3(x(38,46), x(55,86), x(89, 9)),
				      s3(x(12,65), x(61,57), x(94,62)),
				      s3(x(12,46), x(61,86), x(94, 9)) };
		if (memcmp(s, t, sizeof(t)) != 0) {
			fprintf(stderr,
			  "%s:%d:  Different from hard coded mul result.\n",
			  __FILE__, __LINE__);
			return false;
		}
#undef s3
	}

	return true;
}

static bool test_mul()
{
	/* Random checks:  Verify distributivity */

	/* We verify the identity (A + B)*C = A*C + B*C */
	for (int i = 0; i < 1000; ++i) {
		const int dim1 = 5, dim2 = 7, dim3 = 3;

		/* Create matrices */
		Def_mat256_rand(A, a, dim1, dim2, 0xff)
		Def_mat256_rand(B, b, dim1, dim2, 0xff)
		Def_mat256_rand(C, c, dim2, dim3, 0xff)

		/* Compute R1 := (A + B)*C */
		m256v_Def(ApB, apb, dim1, dim2)
		m256v_add(&A, &B, &ApB);
		m256v_Def(R1, r1, dim1, dim3)
		m256v_mul(&ApB, &C, &R1);

		/* Compute R2 := A*C + B*C */
		m256v_Def(AC, ac, dim1, dim3)
		m256v_mul(&A, &C, &AC);
		m256v_Def(BC, bc, dim1, dim3)
		m256v_mul(&B, &C, &BC);
		m256v_Def(R2, r2, dim1, dim3)
		m256v_add(&AC, &BC, &R2);

		/* Make sure R1 = R2 */
		if (memcmp(r1, r2, sizeof(r2)) != 0) {
			fprintf(stderr,
			  "%s:%d:  Multiplication is not distributive.\n",
			  __FILE__, __LINE__);
			return false;
		}
	}

	return true;
}

static void usage()
{
	puts(	"Tests for basic m256v functionality.\n"
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
	RUN_TEST(test_get_el_offs());
	RUN_TEST(test_el_access());
	RUN_TEST(test_swap_rows());
	RUN_TEST(test_clear_row());
	RUN_TEST(test_multadd_row());

	// Tests for the entire matrix
	RUN_TEST(test_iszero());
	RUN_TEST(test_clear());
	RUN_TEST(test_copy());
	RUN_TEST(test_permute_rows());
	RUN_TEST(test_permute_cols());
	RUN_TEST(test_add());
	RUN_TEST(test_add_inplace());
	RUN_TEST(test_mul0());
	RUN_TEST(test_mul());
#undef RUN_TEST

	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
