#include "m2v.h"

extern inline int m2v__get_word(const m2v* M, int r, int c);
extern inline int m2v__get_bit(const m2v* M, int c);
extern inline m2v_base m2v__get_mask(int bit_in_base);

#define get_word	m2v__get_word
#define get_bit		m2v__get_bit
#define get_mask	m2v__get_mask

int m2v_get_row_size(int n_col)
{
	const int bpb = m2v__Bits_per_base;
	return (n_col + bpb - 1) / bpb;
}

m2v m2v_make(int n_row, int n_col, m2v_base* memory)
{
	m2v ret;
	ret.n_row = n_row;
	ret.n_col = n_col;

	ret.row_stride = m2v_get_row_size(n_col);
	ret.e = memory;

	return ret;
}

m2v m2v_get_subview(m2v* M, int row_offs, int col_offs, int n_row, int n_col)
{
	assert(0 <= row_offs);
	assert(0 == col_offs);
	assert(0 <= n_row);
	assert(M->n_col == n_col);
	assert(row_offs + n_row <= M->n_row);

	m2v subv;
	subv.n_row = n_row;
	subv.n_col = M->n_col;
	subv.row_stride = M->row_stride;
	subv.e = M->e + row_offs*M->row_stride;

	return subv;
}

extern inline int m2v_get_el(const m2v* M, int r, int c);
extern inline void m2v_set_el(m2v* M, int r, int c, int val);
extern inline void m2v_toggle_el(m2v* M, int r, int c);

void m2v_swap_rows(m2v* M, int r1, int r2)
{
	assert(0 <= r1 && r1 < M->n_row);
	assert(0 <= r2 && r2 < M->n_row);

	const int o1 = get_word(M, r1, 0);
	const int o2 = get_word(M, r2, 0);
	for (int i = 0; i < M->row_stride; ++i) {
		const m2v_base sw = M->e[o1 + i];
		M->e[o1 + i] = M->e[o2 + i];
		M->e[o2 + i] = sw;
	}
}

void m2v_clear_row(m2v* M, int r)
{
	assert(0 <= r && r < M->n_row);

	const int o1 = get_word(M, r, 0);
	for (int i = 0; i < M->row_stride; ++i) {
		M->e[o1 + i] = 0;
	}
}

void m2v_mult_row(m2v* M, int r, int alpha)
{
	if (alpha == 0) {
		m2v_clear_row(M, r);
	}
}

void m2v_multadd_row(const m2v* M1,
			int r1,
			int alpha,
			m2v* Mt,
			int rt)
{
	assert(M1->n_col == Mt->n_col);
	assert(0 <= r1 && r1 < M1->n_row);
	assert(0 <= rt && rt < Mt->n_row);
	if (alpha == 0)
		return;

	const int o1 = get_word(M1, r1, 0);
	const int ot = get_word(Mt, rt, 0);
	for (int i = 0; i < Mt->row_stride; ++i) {
		Mt->e[ot + i] ^= M1->e[o1 + i];
	}
}

void m2v_multadd_row_from(const m2v* M1,
			int r1,
			int offs,
			int alpha,
			m2v* Mt,
			int rt)
{
	assert(M1->n_col == Mt->n_col);
	assert(0 <= r1 && r1 < M1->n_row);
	assert(0 <= rt && rt < Mt->n_row);
	if (alpha == 0)
		return;

	int o1 = get_word(M1, r1, offs);
	int ot = get_word(Mt, rt, offs);
	/* multadd first partial word */
	{
		const m2v_base m = ~(get_mask(get_bit(Mt, offs)) - 1);
		Mt->e[ot++] ^= M1->e[o1++] & m;
	}

	/* loop over remaining complete words */
	const int oe = get_word(Mt, rt + 1, 0);
	while (ot < oe) {
		Mt->e[ot++] ^= M1->e[o1++];
	}
}

void m2v_copy_row(const m2v* M1, int r1, m2v* Mt, int rt)
{
	assert(M1->n_col == Mt->n_col);
	assert(0 <= r1 && r1 < M1->n_row);
	assert(0 <= rt && rt < Mt->n_row);

	const int o1 = get_word(M1, r1, 0);
	const int ot = get_word(Mt, rt, 0);
	for (int i = 0; i < Mt->row_stride; ++i) {
		Mt->e[ot + i] = M1->e[o1 + i];
	}
}

int m2v_row_iszero(const m2v* M, int r)
{
	assert(0 <= r && r < M->n_row);

	int o = get_word(M, r, 0);
	const int oe = get_word(M, r, M->n_col - 1);

	/* Check all sections but the last */
	while (o < oe) {
		if (M->e[o++] != 0)
			return 0;
	}

	/* Check last section, using correct masking */
	const m2v_base m = (get_mask(get_bit(M, M->n_col - 1)) << 1) - 1;
	if ((M->e[oe] & m) != 0)
		return 0;
	return 1;
}

void m2v_swap_cols(m2v* M, int c1, int c2)
{
	assert(0 <= c1 && c1 < M->n_col);
	assert(0 <= c2 && c2 < M->n_col);

	const int o1 = get_word(M, 0, c1);
	const int o2 = get_word(M, 0, c2);
	const m2v_base m1 = get_mask(get_bit(M, c1));
	const m2v_base m2 = get_mask(get_bit(M, c2));
	if (o1 != o2) {
		for (int i = 0; i < M->n_row; ++i) {
			m2v_base w1 = M->e[i * M->row_stride + o1];
			m2v_base w2 = M->e[i * M->row_stride + o2];
			const int bit1 = !!(w1 & m1);
			const int bit2 = !!(w2 & m2);

			if (bit2) {
				w1 |= m1;
			} else {
				w1 &= ~m1;
			}
			if (bit1) {
				w2 |= m2;
			} else {
				w2 &= ~m2;
			}

			M->e[i * M->row_stride + o1] = w1;
			M->e[i * M->row_stride + o2] = w2;
		}
	} else {
		for (int i = 0; i < M->n_row; ++i) {
			m2v_base w = M->e[i * M->row_stride + o1];
			const int bit1 = !!(w & m1);
			const int bit2 = !!(w & m2);

			if (bit2) {
				w |= m1;
			} else {
				w &= ~m1;
			}
			if (bit1) {
				w |= m2;
			} else {
				w &= ~m2;
			}

			M->e[i * M->row_stride + o1] = w;
		}
	}
}

void m2v_mult_col_from(m2v* M, int c, int offs, int alpha)
{
	assert(0 <= c && c < M->n_col);
	assert(0 <= offs && offs <= M->n_row);

	if (alpha != 0)
		return;
	m2v_base m = ~get_mask(get_bit(M, c));
	for (int r = offs; r < M->n_row; ++r) {
		M->e[get_word(M, r, c)] &= m;
	}
}

void m2v_copy_col(const m2v* M1,
			int c1,
			m2v* Mt,
			int ct)
{
	assert(M1->n_row == Mt->n_row);
	assert(0 <= c1 && c1 < M1->n_col);
	assert(0 <= ct && ct < Mt->n_col);

	const m2v_base m1 = get_mask(get_bit(M1, c1));
	const m2v_base mt = get_mask(get_bit(Mt, ct));
	for (int i = 0; i < M1->n_row; ++i) {
		const m2v_base w1 = M1->e[get_word(M1, i, c1)];
		m2v_base* p = &Mt->e[get_word(Mt, i, ct)];
		if (w1 & m1) {
			*p |= mt;
		} else {
			*p &= ~mt;
		}
	}
}

/* Generic definitions */

#define fadd(a, b)	((a) ^ (b))
#define fsub		((a) ^ (b))
#define flog(a)		(0)
#define fexp(a)		(1)
#define fmul(a, b)	((a) * (b))
#define finv(a)		(1)

#define MV_GEN_TYPE	m2v
#define MV_GEN_PREFIX	m2v
#define MV_GEN_ELTYPE	int
#define MV_GEN_DEFINITIONS
#include "mv_generic.h"
