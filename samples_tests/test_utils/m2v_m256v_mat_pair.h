#ifndef M2V_M256V_MAT_PAIR_H
#define M2V_M256V_MAT_PAIR_H

/**	@file m2v_m256v_mat_pair.h
 *
 *	Helper functions to associate m256v with m2v.
 *
 */

#include <stdbool.h>

#include "m256v.h"
#include "m2v.h"

/** Create an identical pair of matrices, one m2v one m256v.
 *
 *  The binary matrix entries are filled at random.  Buffers are heap
 *  allocated.
 */
void get_mat_pair(int n_row, int n_col, m256v* M256, m2v* M2);
bool check_mat_pair_equal(const m256v* M256, const m2v* M2);
void free_mat_pair_mem(m256v* M256, m2v* M2);

typedef bool (*mat_pair_test)(int nrow, int ncol, m256v* Ml, m2v* Ms);

bool run_mat_pair_test(mat_pair_test testfunc);

void clobber_m2v(m2v* M);

#endif /* M2V_M256V_MAT_PAIR_H */
