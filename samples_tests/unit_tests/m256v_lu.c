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

#define	fadd		gf256_add
#define fmul		gf256_mul

/**	Hard coded LU factorization examples
 */
static bool test_lu0()
{
	int ex = 0;
	int rp[3], cp[3]; // Arrays to store row/column permutations

	/* Example 1 */
	++ex;
	if (1) {
		Def_mat256_init(A, a, 2, 2,
		                1, 1,
			        1, 0 )

		if (m256v_LU_decomp_inplace(&A, rp, cp) != 2) {
			fprintf(stderr, "Error: Example %d: "
			  "LU decomp failed.\n", ex);
			return false;
		}
	}

	/* Example 2 */
	++ex;
	if (1) {
		Def_mat256_init(A, a, 3, 3,
		                1, 1, 0,
			        1, 1, 1,
				0, 1, 0 )

		if (m256v_LU_decomp_inplace(&A, rp, cp) != 3) {
			fprintf(stderr, "Error: Example %d: "
			  "LU decomp failed.\n", ex);
			return false;
		}
	}

	/* Example 3 */
	++ex;
	if (1) {
		Def_mat256_init(A, a, 3, 3,
		                0x20, 0xf6, 0x14,
		                0x89, 0xb0, 0x88,
				0xbc, 0x47, 0x84 )

		int rp[3], cp[3];
		if (m256v_LU_decomp_inplace(&A, rp, cp) != 3) {
			fprintf(stderr, "Error: Example %d: "
			  "LU decomp failed.\n", ex);
			return false;
		}
	}

	/* Example 4 */
	++ex;
	if (1) {
		Def_mat256_init(A, a, 3, 3,
		                0x98, 0x54, 0x4a,
				0xb0, 0xa3, 0x21,
				0xfa, 0x53, 0x3d, )

		if (m256v_LU_decomp_inplace(&A, rp, cp) != 3) {
			fprintf(stderr, "Error: Example %d: "
			  "LU decomp failed.\n", ex);
			return false;
		}
	}

	return true;
}

/** Test LU factorization determinant.
 */
static bool lu_square_inv_test(int dim, int N0, int field)
{
	/* Determine the mask */
	uint8_t mask = get_mask(field);

	/* Invertibility / Determinant tests */
	int i;
	for (i = 0; i < N0; ++i) {
		// Create a random matrix
		uint8_t m_orig[dim * dim];
		rand_arr(m_orig, mask);

		/* Copy to LU decompose */
		uint8_t m[dim * dim];
		memcpy(m, m_orig, dim*dim*sizeof(uint8_t));

		// get the determinant
		int det = -1;
		switch (dim) {
#define det2(i1, i2, i3, i4) fadd(fmul(m[i1], m[i4]), fmul(m[i2], m[i3]))
		case 2:
			det = det2(0, 1, 2, 3);
			break;
		case 3: {
			const uint8_t d1 = det2(4, 5, 7, 8);
			const uint8_t d2 = det2(3, 5, 6, 8);
			const uint8_t d3 = det2(3, 4, 6, 7);
			det = fadd(fadd(fmul(m[0], d1), fmul(m[1], d2)),
					fmul(m[2], d3));
			break;
		}
#undef det2
		};

		int rp[dim], cp[dim];
		m256v A = m256v_make(dim, dim, m);
		int rank = m256v_LU_decomp_inplace(&A, rp, cp);
		if ((rank == dim) != (det != 0)) {
			fprintf(stderr, "Error:  dim=%d, it=%d: "
				"rank=%d, det=%x\n",
				dim, i, rank, (unsigned int)det);
			fprintf(stderr, "        matrix was ");
			for (int u = 0; u < dim * dim; ++u)
				fprintf(stderr, "%hhx ", m_orig[u]);
			fputc('\n', stderr);
			return false;
		}

		/* Compute the determinant of the factorization;
		 * check that it matches the determinant as computed with the
		 * closed formula.
		 *
		 * Note that since characteristic(GF(256)) == 2, we don't
		 * need to take the permutation signs into account.
		 */
		uint8_t lu_det = m256v_LU_det(&A);
		if (lu_det != det) {
			fprintf(stderr, "Error:  Mismatch of the "
			  "computed determinants. (it=%d)\n", i);
			return false;

		}
	}
	return true;
}

static bool test_lu(int field)
{
	if (!lu_square_inv_test(2, 10000, field))
		return false;
	if (!lu_square_inv_test(3, 10000, field))
		return false;
	return true;
}

static bool test_lu_mul_x(bool inplace, int n_row, int n_col, int N, int field)
{
	const uint8_t mask = get_mask(field);
	const int dlen = 8;

	for (int i = 0; i < N; ++i) {
		/* Create a random matrix */
		Def_mat256_rand(A, a, n_row, n_col, mask)
		Def_mat256_rand(Xi, x,
			(inplace && n_row > n_col) ? n_row : n_col,
			dlen, mask)
		m256v X = m256v_make(n_col, dlen, x);

		/* Compute A*X the direct way */
		m256v_Def(Y1, y1, n_row, dlen)
		m256v_mul(&A, &X, &Y1);

		/* Copy A -> LU */
		m256v_Def(LU, lu, n_row, n_col)
		memcpy(lu, a, sizeof(lu));

		/* Calculate LU decomposition */
		int rp[n_row], cp[n_col];
		m256v_LU_decomp_inplace(&LU, rp, cp);

		/* Compute A*X via LU decomposition */
		uint8_t y2_back[n_row * dlen];
		uint8_t* y2;
		if (!inplace) {
			/* non-inplace.  Test m256v_LU_mult */
			m256v Y2 = m256v_make(n_row, dlen, y2_back);
			m256v_LU_mult(&LU, rp, cp, &X, &Y2);

			y2 = y2_back;
		} else {
			/* inplace.  Test m256v_LU_mult_inplace */
			int inv_rp[n_row];
			perm_invert(n_row, rp, inv_rp);
			m256v_LU_mult_inplace(&LU,
						inv_rp,
						cp,
						&Xi);

			y2 = x;
		}

		/* Compare */
		if (memcmp(y1, y2, sizeof(y1)) != 0) {
			fprintf(stderr, "Error:  A*X != PLUQ*X. "
			  "(inplace=%d, n_row=%d, n_col=%d, field=%d, it=%d)\n",
			  inplace, n_row, n_col, field, i);

			/* Print details on the test case */
			fprintf(stderr, "        Details about the "
			  "test case:\n");
			fprintf(stderr, "        rp = ");
			print_int_arr(stderr, n_row, rp);
			fprintf(stderr, "        cp = ");
			print_int_arr(stderr, n_col, cp);
			fprintf(stderr, "        matrix A:\n");
			print_mat(stderr, "        ", &A);
			fprintf(stderr, "        matrix LU:\n");
			print_mat(stderr, "        ", &LU);
			return false;
		}

	}
	return true;
}

static bool test_lu_mul()
{
	for (int inplace = 0; inplace < 2; ++inplace) {
		for (int nrow = 2; nrow < 6; ++nrow) {
			for (int ncol = 2; ncol < 6; ++ncol) {
				const int field_sizes[] = { 2, 256 };
				for (int j = 0;
					j < array_size(field_sizes);
					++j)
				{
					const int fsz = field_sizes[j];
					bool res = test_lu_mul_x(inplace,
								nrow,
								ncol,
								10000,
								fsz);
					if (!res) {
						fprintf(stderr, "Error:  "
						  "Randomized LU "
						  "multiplication "
						  "test failed.\n");
						fprintf(stderr, "        "
						  "inplace=%d "
						  "nrow=%d "
						  "ncol=%d "
						  "field_size=%d\n",
						  inplace, nrow, ncol, fsz);
						return false;
					}
				}
			}
		}
	}

	return true;
}

static bool test_lu_invmul_x(bool inplace, int n_row, int n_col, int N, int field)
{
	const int maxdim = (n_row > n_col ? n_row : n_col);
	const int data_width = 4;
	const uint8_t mask = get_mask(field);

	int ntest = 0;
	for (int i = 0; i < N; ++i) {
		/* Create a random matrix & testing data */
		Def_mat256_rand(A, a, n_row, n_col, mask)
		Def_mat256_rand(X, x, n_col, data_width, mask)

		/* Compute Y := A*X the direct way */
		m256v_Def(Y, y, maxdim, data_width)
		Y.n_row = n_row;
		m256v_mul(&A, &X, &Y);

		/* Calculate LU decomposition */
		int rp[n_row], cp[n_col];
		const int rank = m256v_LU_decomp_inplace(&A, rp, cp);
		++ntest;

		/* Reconstruct X from LU and Y.
		 *
		 * This is not completely possible if the rank of the
		 * matrix is not full.  In such cases, we check if the
		 * difference of X with its reconstructed version is in
		 * the kernel of A.
		 */
		if (inplace) {
			int inv_cp[n_col];
			perm_invert(n_col, cp, inv_cp);

			Y.n_row = maxdim;
			m256v_LU_invmult_inplace(&A, rank, rp, inv_cp, &Y);
			Y.n_row = n_col;
		} else {
			Def_mat256_rand(X1, x1, n_col, data_width, mask)
			m256v_LU_invmult(&A, rank, rp, cp, &Y, &X1);

			/* Copy result back into Y */
			Y.n_row = n_col;
			for (int i = 0; i < n_col; ++i)
				m256v_copy_row(&X1, i, &Y, i);
		}

		/* Check the result */
		m256v_add_inplace(&X, &Y);
		if (rank < n_col) {
			/* If the matrix is not full rank, the resulting
			 * difference is not necessarily zero, but it
			 * must be in the kernel of A.
			 */
			int inv_rp[n_row];
			perm_invert(n_row, rp, inv_rp);
			Y.n_row = maxdim;
			m256v_LU_mult_inplace(&A, inv_rp, cp, &Y);
			Y.n_row = n_row;
		}
		if (!m256v_iszero(&Y)) {
			fprintf(stderr, "Error:  LU inversion gave "
			  "wrong result. (field=%d, it=%d)\n", field, i);
			return false;
		}
	}

	return true;
}

static bool test_lu_invmul()
{
	for (int inplace = 0; inplace < 2; ++inplace) {
		for (int n_row = 2; n_row < 6; ++n_row) {
			for (int n_col = 2; n_col < 6; ++n_col) {
				const int field_sizes[] = { 2, 256 };
				for (int j = 0; j < array_size(field_sizes); ++j) {
					const int fsz = field_sizes[j];
					const bool result =
					  test_lu_invmul_x(inplace,
							  n_row,
							  n_col,
							  10000,
							  fsz);
					if (!result)
					{
							fprintf(stderr, "Error:  "
							  "Randomized LU division "
							  "test failed.\n");
							fprintf(stderr, "        "
							  "inplace=%d "
							  "n_row=%d n_col=%d "
							  "field_size=%d\n",
							  inplace, n_row, n_col, fsz);
							return false;
					}
				}
			}
		}
	}

	return true;
}

static void usage()
{
	puts(	"LU wide implementation tester.\n"
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

	/* Run the various tests */
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

	RUN_TEST(test_lu0());
	RUN_TEST(test_lu(2));
	RUN_TEST(test_lu(256));
	RUN_TEST(test_lu_mul());
	RUN_TEST(test_lu_invmul());
#undef RUN_TEST

	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
