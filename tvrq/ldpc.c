#include <assert.h>

#include "ldpc.h"

void ldpc_generate_mat(m256v* L, const parameters* P)
{
	assert(L->n_row == P->S);
	assert(L->n_col == P->L);

	m256v_clear(L);

	/* Fill in the left part (G_LDPC,1) */
	for (int i = 0; i < P->B; ++i) {
		const int a = 1 + i / P->S;
		int b = i % P->S;
		m256v_set_el(L, b, i, 1);
		b = (b + a) % P->S;
		m256v_set_el(L, b, i, 1);
		b = (b + a) % P->S;
		m256v_set_el(L, b, i, 1);
	}

	/* Add diagonal at offset B */
	for (int i = 0; i < P->S; ++i) {
		m256v_set_el(L, i, i + P->B, 1);
	}

	/* Double diagonal on the right (G_LDPC,2) */
	for (int i = 0; i < P->S; ++i) {
		const int a = i % P->P;
		const int b = (i + 1) % P->P;
		m256v_set_el(L, i, P->W + a, 1);
		m256v_set_el(L, i, P->W + b, 1);
	}
}
