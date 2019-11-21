#ifndef M256V_H
#define M256V_H

/**	@file m256v.h
 *
 *	Matrix views over GF256.
 */

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
	int n_row;		/* number of rows */
	int n_col;		/* number of cols */

	size_t rstride;		/* row stride */

	uint8_t* e;		/* storage for the matrix entries */
} m256v;

/** \defgroup Initialisation		Initialisation */
/*@{*/

/**	Initialize a basic row-major view.
 *
 *	This initializes the size and stride elements of the matrix, but
 *	not the backing storage.  Backing storage is handled by the
 *	user, m256v_* routines do not allocate or free that memory ever.
 */
m256v m256v_make(int n_row, int n_col, uint8_t* memory);

/**	Create a view for a submatrix.
 *
 *	Creates a view for the given region of the matrix M.
 */
m256v m256v_get_subview(const m256v* M,
				int row_offs,
				int col_offs,
				int n_row,
				int n_col);

/**	Create a view with backend storage on the stack.
 */
#define m256v_Def(mv_name, backend_name, n_row, n_col) \
		uint8_t backend_name[(n_row) * (n_col)]; \
		m256v mv_name = m256v_make((n_row), (n_col), backend_name);

/*@}*/

/** \defgroup ElementAccess		Element access */
/*@{*/

/**	Compute the address offset of an element.
 *
 *	This function is specific to m256v and not required for
 *	mv_generic.
 */
inline size_t m256v_get_el_offs(const m256v* M, int r, int c);

inline uint8_t m256v_get_el(const m256v* M, int r, int c);
inline void m256v_set_el(m256v* M, int r, int c, uint8_t val);

/*@}*/
/** \defgroup ElementaryRowOps		Elementary row operations */
/*@{*/

void m256v_swap_rows(m256v* M, int r1, int r2);
void m256v_clear_row(m256v* M, int r);
void m256v_mult_row(m256v* M, int r, uint8_t alpha);
void m256v_multadd_row(const m256v* M1,
			int r1,
			uint8_t alpha,
			m256v* Mt,
			int rt);
void m256v_multadd_row_from(const m256v* M1,
			int r1,
			int offs,
			uint8_t alpha,
			m256v* Mt,
			int rt);
void m256v_copy_row(const m256v* M1,
			int r1,
			m256v* Mt,
			int rt);
int m256v_row_iszero(const m256v* M, int r);

/*@}*/
/** \defgroup ElementaryColOps		Elementary column operations */
/*@{*/

void m256v_swap_cols(m256v* M, int c1, int c2);
void m256v_mult_col_from(m256v* M, int c, int offs, uint8_t alpha);
void m256v_copy_col(const m256v* M1,
			int c1,
			m256v* Mt,
			int ct);
/*@}*/

/* Inline definitions */
#ifndef PY_CFFI

inline size_t m256v_get_el_offs(const m256v* M, int r, int c)
{
	assert(0 <= r);
	assert(0 <= c);
	assert(r < M->n_row);
	/* The check for the column is loose so as to make it possible
	 * to get a pointer to one beyond the last column
	 */
	assert(c <= M->n_col);

	return r * M->rstride + c;
}

inline uint8_t m256v_get_el(const m256v* M, int r, int c)
{
	return M->e[m256v_get_el_offs(M, r, c)];
}

inline void m256v_set_el(m256v* M, int r, int c, uint8_t val)
{
	M->e[m256v_get_el_offs(M, r, c)] = val;
}

#endif

/* Add prototypes for generically implemented matrix operations */
#define MV_GEN_TYPE	m256v
#define MV_GEN_PREFIX	m256v
#define MV_GEN_ELTYPE	uint8_t
#include "mv_generic.h"
#undef MV_GEN_TYPE
#undef MV_GEN_PREFIX
#undef MV_GEN_ELTYPE

#endif /* M256V_H */
