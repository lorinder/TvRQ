#include <assert.h>

#include "hdpc.h"
#include "ldpc.h"
#include "lt.h"
#include "rq_matrix.h"

void rq_matrix_get_dim(const parameters* P,
			int n_ESIs,
			int* n_rows_out,
			int* n_cols_out)
{
	const int n_rows = n_ESIs + (P->Kprime - P->K) + P->S + P->H;
	const int n_cols = P->L;

	if (n_rows_out)
		*n_rows_out = n_rows;
	if (n_cols_out)
		*n_cols_out = n_cols;
}

void rq_matrix_generate(m256v* M,
			const parameters* P,
			int n_ESIs,
			const uint32_t* ESIs)
{
	/* Check the matrix dimension */
	int n_rows, n_cols;
	rq_matrix_get_dim(P, n_ESIs, &n_rows, &n_cols);
	assert(n_rows == M->n_row);
	assert(n_cols == M->n_col);

	/* Write LT part of the matrix */
	int rowoffs = 0;
	{
		const int n_pad = P->Kprime - P->K;
		const int n_ISIs = n_ESIs + n_pad;

		/* Convert ESIs to ISIs, append padding symbols */
		uint32_t ISIs[n_ISIs];
		for (int i = 0; i < n_ESIs; ++i)
			ISIs[i] = ESIs[i] + (ESIs[i] >= P->K ? n_pad : 0);
		for (int i = 0; i < n_pad; ++i)
			ISIs[n_ESIs + i] = P->K + i;

		/* Write LT matrix */
		m256v LT = m256v_get_subview(M, 0, 0, n_ISIs, P->L);
		lt_generate_mat(&LT, P, n_ISIs, ISIs);

		rowoffs += n_ISIs;
	}

	/* Write LDPC part of the matrix */
	m256v LDPC = m256v_get_subview(M, rowoffs, 0, P->S, P->L);
	ldpc_generate_mat(&LDPC, P);
	rowoffs += P->S;

	/* Generate HDPC part of the matrix */
	m256v HDPC = m256v_get_subview(M, rowoffs, 0, P->H, P->L);
	hdpc_generate_mat(&HDPC, P);
	rowoffs += P->H;

	assert(rowoffs == n_rows);
}
