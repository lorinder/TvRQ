#include <assert.h>

#include "lt.h"
#include "tuple.h"

// Sect 5.3.5.3
void lt_generate_mat(m256v* M,
			const parameters* P,
			int n_ISIs,
			const uint32_t* ISIs)
{
	assert(M->n_row == n_ISIs);
	assert(M->n_col == P->L);

	m256v_clear(M);
	for (int i = 0; i < n_ISIs; ++i) {
		tuple T = tuple_generate_from_ISI(ISIs[i], P);
		m256v_set_el(M, i, T.b, 1);
		for (int j = 1; j < T.d; ++j) {
			T.b = (T.b + T.a) % P->W;
			m256v_set_el(M, i, T.b, 1);
		}
		while (T.b1 >= P->P)
			T.b1 = (T.b1 + T.a1) % P->P1;
		m256v_set_el(M, i, P->W + T.b1, 1);
		for (int j = 1; j < T.d1; ++j) {
			T.b1 = (T.b1 + T.a1) % P->P1;
			while (T.b1 >= P->P)
				T.b1 = (T.b1 + T.a1) % P->P1;
			m256v_set_el(M, i, P->W + T.b1, 1);
		}
	}
}
