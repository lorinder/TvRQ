#include <assert.h>
#include <stdlib.h>

#include "m2v_m256v_mat_pair.h"
#include "utils.h"

void get_mat_pair(int n_row,
			int n_col,
			m256v* M256,
			m2v* M2)
{
	uint8_t* buf256 = malloc(n_row * n_col * sizeof(uint8_t));
	*M256 = m256v_make(n_row, n_col, buf256);


	m2v_base* buf2 = malloc(n_row * m2v_get_row_size(n_col)
				* sizeof(m2v_base));
	*M2 = m2v_make(n_row, n_col, buf2);

	/* Fill in random data */
	for (int r = 0; r < n_row; ++r) {
		for (int c = 0; c < n_col; ++c) {
			const int v = (rand() >> 8) & 1;
			m256v_set_el(M256, r, c, v);
			m2v_set_el(M2, r, c, v);
		}
	}
}

bool check_mat_pair_equal(const m256v* M256,
				const m2v* M2)
{
	assert(M256->n_row == M2->n_row);
	assert(M256->n_col == M2->n_col);
	for (int r = 0; r < M256->n_row; ++r) {
		for (int c = 0; c < M2->n_col; ++c) {
			if (m256v_get_el(M256, r, c) != m2v_get_el(M2, r, c))
				return false;
		}
	}
	return true;
}

void free_mat_pair_mem(m256v* M256, m2v* M2)
{
	free(M2->e);
	free(M256->e);
}

bool run_mat_pair_test(mat_pair_test testfunc)
{
	bool succ = true;

	static const int nrow_arr[] = { 17, 41 };
	for (int i = 0; i < array_size(nrow_arr); ++i) {
		const int nrow = nrow_arr[i];
		static const int ncol_arr[] = { 7, 15, 32, 33, 99 };
		for (int j = 0; j < array_size(ncol_arr); ++j) {
			const int ncol = ncol_arr[j];
			m256v Ml;
			m2v Ms;
			get_mat_pair(nrow, ncol, &Ml, &Ms);

			if(!testfunc(nrow, ncol, &Ml, &Ms))
				succ = false;

			if (!check_mat_pair_equal(&Ml, &Ms))
				succ = false;
			free_mat_pair_mem(&Ml, &Ms);
		}
	}

	return succ;
}

void clobber_m2v(m2v* M)
{
	const int n_word = M->n_row * M->row_stride;
	for (int i = 0; i < n_word; ++i) {
		M->e[i] = rand();
	}
}
