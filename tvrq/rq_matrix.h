#ifndef RQ_MATRIX_H
#define RQ_MATRIX_H

/**	@file rq_matrix.h
 *
 *	Create the entire RQ matrix, i.e., the matrix that gives the
 *	linear relationship between the intermediate block and the
 *	symbols, including LDPC and HDPC symbols.
 */

#include <stdint.h>

#include "m256v.h"
#include "parameters.h"

void rq_matrix_get_dim(const parameters* P,
			int n_ESIs,
			int* n_rows_out,
			int* n_cols_out);

void rq_matrix_generate(m256v* M,
			const parameters* P,
			int n_ESIs,
			const uint32_t* ESIs);

#endif /* RQ_MATRIX_H */
