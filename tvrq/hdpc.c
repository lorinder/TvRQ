#include <assert.h>
#include <stdlib.h>

#include "gf256.h"
#include "hdpc.h"
#include "rand.h"

// Section 5.3.3.3.
void hdpc_generate_mat_specexact(m256v* H, const parameters* P)
{
	assert(H->n_row == P->H);
	assert(H->n_col == P->L);

	/* Create the MT matrix */
	uint8_t* mt = malloc(P->H * (P->Kprime + P->S));
	m256v MT = m256v_make(P->H, P->Kprime + P->S, mt);
	m256v_clear(&MT);
	for (int j = 0; j < P->Kprime + P->S - 1; ++j) {
		uint32_t a = Rand(j + 1, 6, P->H);
		uint32_t b = (a + Rand(j + 1, 7, P->H - 1) + 1) % P->H;
		m256v_set_el(&MT, a, j, 1);
		m256v_set_el(&MT, b, j, 1);
	}
	uint8_t val = 1;
	for (int j = 0; j < P->H; ++j) {
		m256v_set_el(&MT, j, P->Kprime + P->S - 1, val);
		val = gf256_mul(val, 2);
	}

	/* Create the GAMMA matrix */
	const int d = P->Kprime + P->S;
	uint8_t* gamma = malloc(d * d);
	m256v GAMMA = m256v_make(d, d, gamma);
	m256v_clear(&GAMMA);

	val = 1;
	for (int i = 0; i < d; ++i) {
		for (int j = i; j < d; ++j) {
			m256v_set_el(&GAMMA, j, j - i, val);
		}
		val = gf256_mul(val, 2);
	}

	/* Place the product MT * GAMMA into the G_HDPC matrix */
	m256v G_HDPC = m256v_get_subview(H, 0, 0, P->H, P->Kprime + P->S);
	m256v_mul(&MT, &GAMMA, &G_HDPC);

	/* Write the diagonal matrix I_H */
	m256v I_H = m256v_get_subview(H, 0, P->Kprime + P->S, P->H, P->H);
	m256v_clear(&I_H);
	for (int i = 0; i < P->H; ++i) {
		m256v_set_el(&I_H, i, i, 1);
	}

	/* Free temp space */
	free(gamma);
	free(mt);
}

void hdpc_generate_mat_faster(m256v* H, const parameters* P)
{
	assert(H->n_row == P->H);
	assert(H->n_col == P->L);

	/* Create G_HDPC */
	m256v G_HDPC = m256v_get_subview(H, 0, 0, P->H, P->Kprime + P->S);
	uint8_t val = 1;
	for (int j = 0; j < P->H; ++j) {
		m256v_set_el(&G_HDPC, j, P->Kprime + P->S - 1, val);
		val = gf256_mul(val, 2);
	}
	for (int j = P->Kprime + P->S - 2; j >= 0; --j) {
		/* Copy over previous column, multiplied by alpha */
		for (int i = 0; i < P->H; ++i) {
			m256v_set_el(&G_HDPC, i, j,
			  gf256_mul(2, m256v_get_el(&G_HDPC, i, j + 1)));
		}

		/* Add in the two flipped positions */
		int a = Rand(j + 1, 6, P->H);
		m256v_set_el(&G_HDPC, a, j,
		  gf256_add(1, m256v_get_el(&G_HDPC, a, j)));
		a = (a + Rand(j + 1, 7, P->H - 1) + 1) % P->H;
		m256v_set_el(&G_HDPC, a, j,
		  gf256_add(1, m256v_get_el(&G_HDPC, a, j)));
	}

	/* Create I_H */
	m256v I_H = m256v_get_subview(H, 0, P->Kprime + P->S, P->H, P->H);
	m256v_clear(&I_H);
	for (int i = 0; i < P->H; ++i) {
		m256v_set_el(&I_H, i, i, 1);
	}
}
