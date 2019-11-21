#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gf256.h"
#include "m256v.h"

/* Shorten symbol names for simpler code */
#define get_el_offs	m256v_get_el_offs
#define get_el		m256v_get_el
#define set_el		m256v_set_el

#define fadd		gf256_add
#define fsub		gf256_sub
#define flog		gf256_log
#define fexp		gf256_exp
#define fmul		gf256_mul
#define finv		gf256_inv

m256v m256v_make(int n_row, int n_col, uint8_t* memory)
{
	m256v ret;
	ret.n_row = n_row;
	ret.n_col = n_col;
	ret.rstride = n_col;
	ret.e = memory;
	return ret;
}

m256v m256v_get_subview(const m256v* M,
				int row_offs,
				int col_offs,
				int n_row,
				int n_col)
{
	assert(0 <= row_offs);
	assert(row_offs + n_row <= M->n_row);
	assert(0 <= col_offs);
	assert(col_offs + n_col <= M->n_col);

	m256v R;
	R.n_row = n_row;
	R.n_col = n_col;
	R.rstride = M->rstride;
	R.e = M->e + m256v_get_el_offs(M, row_offs, col_offs);

	return R;
}

extern inline size_t m256v_get_el_offs(const m256v* M, int r, int c);
extern inline uint8_t m256v_get_el(const m256v* M, int r, int c);
extern inline void m256v_set_el(m256v* M, int r, int c, uint8_t val);

void m256v_swap_rows(m256v* M, int r1, int r2)
{
	if (r1 == r2)
		return;

	size_t o1 = get_el_offs(M, r1, 0);
	size_t o2 = get_el_offs(M, r2, 0);
	for (int i = 0; i < M->n_col; ++i) {
		const uint8_t sw = M->e[o1];
		M->e[o1] = M->e[o2];
		M->e[o2] = sw;
		++o1;
		++o2;
	}
}

void m256v_clear_row(m256v* M, int r)
{
	for (int i = 0; i < M->n_col; ++i)
		set_el(M, r, i, 0);
}

void m256v_mult_row(m256v* M, int r, uint8_t alpha)
{
	/* Special case:  Zero multiplier */
	if (alpha == 0) {
		m256v_clear_row(M, r);
		return;
	}

	/* General case:  Nonzero alpha */
	const uint8_t log_alpha = flog(alpha);
	for (int j = 0; j < M->n_col; ++j) {
		uint8_t v = get_el(M, r, j);
		if (v != 0) {
			set_el(M, r, j, fexp(flog(v) + log_alpha));
		}
	}
}

void m256v_multadd_row(const m256v* M1,
			int r1,
			uint8_t alpha,
			m256v* Mt,
			int rt)
{
	m256v_multadd_row_from(M1, r1, 0, alpha, Mt, rt);
}

void m256v_multadd_row_from(const m256v* M1,
			int r1,
			int offs,
			uint8_t alpha,
			m256v* Mt,
			int rt)
{
	assert (M1->n_col == Mt->n_col);

	if (alpha == 0)
		return;
	if (alpha == 1) {
		uint8_t* s = M1->e + get_el_offs(M1, r1, offs);
		uint8_t* t = Mt->e + get_el_offs(Mt, rt, offs);
		uint8_t* end = M1->e + get_el_offs(M1, r1, M1->n_col);
		while (s < end) {
			*t++ ^= *s++;
		}
	} else {
		const int log_alpha = flog(alpha);
		uint8_t* s = M1->e + get_el_offs(M1, r1, offs);
		uint8_t* t = Mt->e + get_el_offs(Mt, rt, offs);
		uint8_t* end = M1->e + get_el_offs(M1, r1, M1->n_col);
		while (s < end) {
			/* Mt[rt, i] += exp(log_alpha + log(M1[r1, i]))
			 * if said logarithm is defined.
			 *
			 * Otherwise, M[rt, i] is unchanged.
			 */
			const uint8_t ve = *s++;
			if (ve != 0) {
				*t ^= fexp(log_alpha + flog(ve));
			}
			++t;
		}
	}
}

void m256v_copy_row(const m256v* M1,
			int r1,
			m256v* Mt,
			int rt)
{
	assert (M1->n_col == Mt->n_col);

	for (int i = 0; i < Mt->n_col; ++i) {
		set_el(Mt, rt, i, get_el(M1, r1, i));
	}
}

int m256v_row_iszero(const m256v* M, int r)
{
	for (int c = 0; c < M->n_col; ++c) {
		if (get_el(M, r, c) != 0) {
			return 0;
		}
	}
	return 1;
}

void m256v_swap_cols(m256v* M, int c1, int c2)
{
	if (c1 == c2)
		return;

	size_t o1 = get_el_offs(M, 0, c1);
	size_t o2 = get_el_offs(M, 0, c2);
	for (int i = 0; i < M->n_row; ++i) {
		const uint8_t sw = M->e[o1];
		M->e[o1] = M->e[o2];
		M->e[o2] = sw;
		o1 += M->rstride;
		o2 += M->rstride;
	}
}

void m256v_mult_col_from(m256v* M, int c, int offs, uint8_t alpha)
{
	if (alpha != 0) {
		const int log_alpha = flog(alpha);
		for (int j = offs; j < M->n_row; ++j) {
			uint8_t val = get_el(M, j, c);
			if (val != 0) {
				val = fexp(flog(val) + log_alpha);
				set_el(M, j, c, val);
			}
		}
	} else {
		for (int j = offs; j < M->n_row; ++j) {
			set_el(M, j, c, 0);
		}
	}
}

void m256v_copy_col(const m256v* M1,
			int c1,
			m256v* Mt,
			int ct)
{
	assert (M1->n_row == Mt->n_row);

	for (int i = 0; i < Mt->n_row; ++i) {
		set_el(Mt, i, ct, get_el(M1, i, c1));
	}
}

#define MV_GEN_TYPE	m256v
#define MV_GEN_PREFIX	m256v
#define MV_GEN_ELTYPE	uint8_t
#define MV_GEN_DEFINITIONS
#include "mv_generic.h"
