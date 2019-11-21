#ifndef M2V_H
#define M2V_H

/**	@file m2v.h
 *
 *	Matrix views over GF2.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

typedef unsigned int m2v_base;

typedef struct {
	int n_row;		/* number of rows */
	int n_col;		/* number of columns */

	int row_stride;		/* number of m2v_base used per row */

	m2v_base* e;
} m2v;

int m2v_get_row_size(int n_col);

m2v m2v_make(int n_row, int n_col, m2v_base* memory);
m2v m2v_get_subview(m2v* M, int row_offs, int col_offs, int n_row, int n_col);

#define m2v_Def(mv_name, storage_name, n_row, n_col) \
	m2v_base storage_name[(n_row) * m2v_get_row_size(n_col)]; \
	m2v mv_name = m2v_make((n_row), (n_col), storage_name);

inline int m2v_get_el(const m2v* M, int r, int c);
inline void m2v_set_el(m2v* M, int r, int c, int val);
inline void m2v_toggle_el(m2v* M, int r, int c);

void m2v_swap_rows(m2v* M, int r1, int r2);
void m2v_clear_row(m2v* M, int r);
void m2v_mult_row(m2v* M, int r, int alpha);
void m2v_multadd_row(const m2v* M1,
			int r1,
			int alpha,
			m2v* Mt,
			int rt);
void m2v_multadd_row_from(const m2v* M1,
			int r1,
			int offs,
			int alpha,
			m2v* Mt,
			int rt);
void m2v_copy_row(const m2v* M1,
			int r1,
			m2v* Mt,
			int rt);
int m2v_row_iszero(const m2v* M, int r);

void m2v_swap_cols(m2v* M, int c1, int c2);
void m2v_mult_col_from(m2v* M, int c, int offs, int alpha);
void m2v_copy_col(const m2v* M1,
			int c1,
			m2v* Mt,
			int ct);

/* Inline definitions */
#ifndef PY_CFFI

/* Helpers */

#define m2v__Bits_per_base  (sizeof(m2v_base) * CHAR_BIT)

inline int m2v__get_word(const m2v* M, int r, int c)
{
	const int bpb = m2v__Bits_per_base;
	return M->row_stride * r + c / bpb;
}

inline int m2v__get_bit(const m2v* M, int c)
{
	const int bpb = m2v__Bits_per_base;
	return c % bpb;
}

inline m2v_base m2v__get_mask(int bit_in_base)
{
	return ((m2v_base)1) << bit_in_base;
}

/* Primitives that need to be fast */

inline int m2v_get_el(const m2v* M, int r, int c)
{
	assert(0 <= r && r < M->n_row);
	assert(0 <= c && c < M->n_col);
	const int w = m2v__get_word(M, r, c);
	const int b = m2v__get_bit(M, c);
	return !!(M->e[w] & m2v__get_mask(b));
}

inline void m2v_set_el(m2v* M, int r, int c, int val)
{
	assert(0 <= r && r < M->n_row);
	assert(0 <= c && c < M->n_col);
	const int w = m2v__get_word(M, r, c);
	const int b = m2v__get_bit(M, c);
	const m2v_base m = m2v__get_mask(b);
	if (val) {
		M->e[w] |= m;
	} else {
		M->e[w] &= ~m;
	}
}

inline void m2v_toggle_el(m2v* M, int r, int c)
{
	assert(0 <= r && r < M->n_row);
	assert(0 <= c && c < M->n_col);
	const int w = m2v__get_word(M, r, c);
	const int b = m2v__get_bit(M, c);
	const m2v_base m = m2v__get_mask(b);
	M->e[w] ^= m;
}

#endif

#define MV_GEN_TYPE	m2v
#define MV_GEN_PREFIX	m2v
#define MV_GEN_ELTYPE	int
#include "mv_generic.h"
#undef MV_GEN_ELTYPE
#undef MV_GEN_PREFIX
#undef MV_GEN_TYPE


#endif /* M2V_H */
