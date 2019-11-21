/**	@file mv_generic.h
 *
 *	Generic implementation of matrix view operations.  This file is
 *	is included by the specialized C file to create generic
 *	implementations, with the appropriate MV_GEN_* macros defined.
 *	It's not otherwise useful as a header file.
 *
 *	There is no header guard, since this file can in principle be
 *	included several times.
 */

#ifndef MV_GEN_TYPE
#error MV_GEN_TYPE needs to be defined when including mv_generic.h
#endif

#ifndef MV_GEN_PREFIX
#error MV_GEN_PREFIX needs to be defined when including mv_generic.h
#endif

#ifndef MV_GEN_ELTYPE
#define MV_GEN_ELTYPE needs to be defined when including mv_generic.h
#endif

#define MV_GEN_N(b)		MV_GEN_CAT(MV_GEN_PREFIX, b)

#define MV_GEN_CAT(a, b)	MV_GEN_CAT_(a, b)
#define MV_GEN_CAT_(a, b)	a ## b

/** \defgroup FullMatrixOps		Elementary Operations on entire matrices */
/*@{*/

/** Check if a matrix is all zero.
 *
 *  Checks if a matrix is zero, i.e. does not contain any nonzero
 *  entries.
 */
int MV_GEN_N(_iszero)(const MV_GEN_TYPE* A);

/** Clear a matrix.
 *
 *  Clears a matrix, sets all entries to zero.
 */
void MV_GEN_N(_clear)(MV_GEN_TYPE* A);

/** Copy a matrix.
 *
 *  Copy matrix A into B.  It is assumed that A and B have the same
 *  size.
 */
void MV_GEN_N(_copy)(const MV_GEN_TYPE* A, MV_GEN_TYPE* B);

/** Copy a rectangular submatrix of A into At.
 *
 *  Copies the given rectangular submatrix of A into the matrix At,
 *  starting at offset (At_row_offs, At_col_offs).  Elements in At not
 *  within the copied matrix remain unchanged.
 *
 *  This algorithm is element-by-element and thus not necessarily very
 *  fast.
 *
 *  @param	A
 *		origin matrix
 *
 *  @param	A_row_offs
 *		Row offset of the submatrix in A to copy
 *
 *  @param	A_col_offs
 *		Column offset of the submatrix in A to copy
 *
 *  @param	At
 *		Target matrix.
 *
 *  @param      At_row_offs
 *		Row offset where the copied submatrix will be placed in
 *		At.
 *
 *  @param	At_col_offs
 *		Column offset where the copied submatrix will be placed
 *		in At.
 */
void MV_GEN_N(_copy_submat)(const MV_GEN_TYPE* A,
			int A_row_offs,
			int A_col_offs,
			int n_row,
			int n_col,
			MV_GEN_TYPE* At,
			int At_row_offs,
			int At_col_offs);

/** Permute the rows of M in-place.
 *
 *  Permutes the rows of M according to rowperm which is assumed in
 *  "from" format, i.e., row rowperm[i] will be moved to row i.
 *
 *  @param	M
 *		Matrix to operate on.
 *
 *  @param	rowperm
 *		Permutation to execute, in "from" format.
 */
void MV_GEN_N(_permute_rows)(MV_GEN_TYPE* M, const int* rowperm);

/** Permute the columns of M in-place.
 *
 *  Permutes the columns of M according to colperm.
 *
 *  @param	M
 *		Matrix to operate on.
 *
 *  @param	colperm
 *		Permutation to execute, in "from" format.
 */
void MV_GEN_N(_permute_cols)(MV_GEN_TYPE* M, const int* colperm);

void MV_GEN_N(_add)(const MV_GEN_TYPE* A, const MV_GEN_TYPE* B, MV_GEN_TYPE* AplusB_out);
void MV_GEN_N(_add_inplace)(const MV_GEN_TYPE *A, MV_GEN_TYPE* B_inout);
void MV_GEN_N(_mul)(const MV_GEN_TYPE* A, const MV_GEN_TYPE* B, MV_GEN_TYPE* AB_out);

/*@}*/

/* \defgroup LuDecomp			LU decomposition */
/*@{*/

/** Compute the LU decomposition of A in-place.
 *
 *  Computes a factorization PLUQ of A.  P and Q are permutation
 *  matrices, L is lower triangular and U is upper triangular.
 *
 *  In this implementation, the matrix A need not be square.  If A is
 *  rectangular, then one of the factors L or U will also be
 *  rectangular.
 *
 *  The matrix need not be full rank.  The subsequent operation of
 *  forward multiplication can also be performed if the matrix is not
 *  full rank.  Inverse computation can also be performed on non-full
 *  rank matrices; in such cases the inverse is not unique.
 *
 *  The choice of pivot is made in such a way that row permutations are
 *  preferred over column permutations whenever possible.  In
 *  particular, if the matrix has at least as many rows as columns and
 *  is full rank, then no column permutation is performed.
 *
 *  @param	A
 *		The matrix to decompose.  After execution of the
 *		routine, the matrix will have the same dimensions, but
 *		contain the LU decomposition.
 *
 *  @param	rowperm
 *		An array having space for n_row members, where n_row is
 *		the number of rows of A.  This array will contain the
 *		row permutation performed on A.  The permutation is
 *		given in the "from" form, i.e., to repeat it, move the
 *		row rowperm[i] in the original matrix to row i in
 *		the permuted matrix.
 *
 *  @param	colperm
 *		An array having space for n_col members, where n_col is
 *		the number of columns of A.  This array will contain the
 *		column permutation of A.  The permutation is given in
 *		the "from" form.
 *
 *  @return	rank	the rank of the LU decomposed matrix.
 */
int MV_GEN_N(_LU_decomp_inplace)(MV_GEN_TYPE* A,
			int* rowperm,
			int* colperm);

/** Compute the determinant of a square LU decomposed matrix.
 *
 *  This computes the determinant of the LU decomposition (which is the
 *  same as the determinant of A).  Notice that since the GF(256) has
 *  even characteristic, the row permutation and column permutation have
 *  determinant 1, thus they don't affect the determinant.
 */
MV_GEN_ELTYPE MV_GEN_N(_LU_det)(const MV_GEN_TYPE* LU);

/** Compute Y := PLUQ*X for given X.
 *
 *  This implements forward matrix multiplication on an LU
 *  factored matrix.  This version is out-of-place, i.e. the result
 *  vector Y and the input vector X are distinct.
 *
 *  @param	rowperm
 *		The row permutation as computed by the LU decomposition
 *		algorithm.  Must be non-NULL.
 *
 *  @param	colperm
 *		The column permutation as computed by the LU
 *		decomposition algorithm.  Must be non-NULL.
 *
 *  @param	X
 *		The matrix X.
 *
 *  @param	Y_out
 *		The output matrix Y.
 */
void MV_GEN_N(_LU_mult)(const MV_GEN_TYPE* LU,
			const int* rowperm,
			const int* colperm,
			const MV_GEN_TYPE* X,
			MV_GEN_TYPE* Y_out);

void MV_GEN_N(_LU_invmult)(const MV_GEN_TYPE* LU,
			int rank,
			const int* rowperm,
			const int* colperm,
			const MV_GEN_TYPE* Y,
			MV_GEN_TYPE* X_out);

/** Compute PLUQ*X for given X in-place.
 *
 *  @param	LU
 *		The LU decomposition.
 *
 *  @param	inv_rowperm
 *		The inverse of the row permutation as computed by the LU
 *		decomposition algorithm.  If NULL, no row permutation is
 *		applied.
 *
 *  @param	colperm
 *		The column permutation as computed by the LU
 *		decomposition algorithm.  If NULL, no column permutation
 *		is applied.
 */
void MV_GEN_N(_LU_mult_inplace)(const MV_GEN_TYPE* LU,
			const int* inv_rowperm,
			const int* colperm,
			MV_GEN_TYPE* X_inout);

/** Find X such that PLUQ*X = Y, inplace.
 *
 *  This algorithm assumes that a corresponding X exists; it is not
 *  checked whether that holds.  Garbage output is produced if that
 *  assumption does not hold.
 *
 *  @param	LU
 *		The LU decomposition.
 *
 *  @param	rank
 *		The rank of the A matrix.  This quantity is determined
 *		by the LU decomposition.  If -1 is given, full rank is
 *		assumed.
 *
 *  @param	rowperm
 *		The row permutation as determined by the LU
 *		decomposition algorithm.
 *
 *  @param	inv_colperm
 *		The inverse of the column permutation.
 *
 *  @param	X_inout
 *		The X input matrix and the Y output matrix.  The number
 *		of rows of this matrix must be the larger of the two
 *		matrices.
 */
void MV_GEN_N(_LU_invmult_inplace)(const MV_GEN_TYPE* LU,
			int rank,
			const int* rowperm,
			const int* inv_colperm,
			MV_GEN_TYPE* X_inout);

void MV_GEN_N(_L_mult_inplace)(const MV_GEN_TYPE* LU, MV_GEN_TYPE* X_inout);
void MV_GEN_N(_U_mult_inplace)(const MV_GEN_TYPE* LU, MV_GEN_TYPE* X_inout);
void MV_GEN_N(_L_invmult_inplace)(const MV_GEN_TYPE* LU, int rank, MV_GEN_TYPE* X_inout);
void MV_GEN_N(_U_invmult_inplace)(const MV_GEN_TYPE* LU, int rank, MV_GEN_TYPE* X_inout);

void MV_GEN_N(_L_invmult_inplace_p)(const MV_GEN_TYPE* LU,
					int rank,
					MV_GEN_TYPE* X_inout,
					const int* placements);
void MV_GEN_N(_U_invmult_inplace_p)(const MV_GEN_TYPE* LU,
					int rank,
					MV_GEN_TYPE* X_inout,
					const int* placements);

/*@}*/


/*
 * The actual implementations; provided if MV_GEN_DEFINITIONS is set.
 */

#ifdef MV_GEN_DEFINITIONS

#if !defined(fadd) || !defined(fsub) || !defined(fmul) || !defined(finv)
#error field arithmetic operation macros fadd, fsub, fmul, finv must be defined.
#endif
#if !defined(fexp) || !defined(flog)
#error field arithmetic operation macros flog and fexp must be defined.
#endif

/* Helpful macros */
#define SwapInt(a, b) SwapInt_(a, b, SwapInt__swval)
#define SwapInt_(a, b,			s) \
	do { \
		const int s = (b); \
		(b) = (a); \
		(a) = s; \
	} while(0)


int MV_GEN_N(_iszero)(const MV_GEN_TYPE* M)
{
	for (int r = 0; r < M->n_row; ++r) {
		if (!MV_GEN_N(_row_iszero)(M, r)) {
			return 0;
		}
	}
	return 1;
}

void MV_GEN_N(_clear)(MV_GEN_TYPE* M)
{
	for (int i = 0; i < M->n_row; ++i) {
		MV_GEN_N(_clear_row)(M, i);
	}
}

void MV_GEN_N(_copy)(const MV_GEN_TYPE* A, MV_GEN_TYPE* B)
{
	assert(A->n_row == B->n_row);
	assert(A->n_col == B->n_col);
	for (int i = 0; i < A->n_row; ++i) {
		MV_GEN_N(_copy_row)(A, i, B, i);
	}
}

void MV_GEN_N(_copy_submat)(const MV_GEN_TYPE* A,
				int A_row_offs,
				int A_col_offs,
				int n_row,
				int n_col,
				MV_GEN_TYPE* At,
				int At_row_offs,
				int At_col_offs)
{
	assert(0 <= n_row);
	assert(0 <= n_col);

	assert(0 <= A_row_offs);
	assert(0 <= A_col_offs);
	assert(A_row_offs + n_row <= A->n_row);
	assert(A_col_offs + n_col <= A->n_col);

	assert(0 <= At_row_offs);
	assert(0 <= At_col_offs);
	assert(At_row_offs + n_row <= At->n_row);
	assert(At_col_offs + n_col <= At->n_col);

	for (int r = 0; r < n_row; ++r) {
		for (int c = 0; c < n_col; ++c) {
			MV_GEN_N(_set_el)(At,
			  r + At_row_offs,
			  c + At_col_offs,
			  MV_GEN_N(_get_el)(A, r + A_row_offs, c + A_col_offs));
		}
	}
}

void MV_GEN_N(_permute_rows)(MV_GEN_TYPE* M, const int* rowperm)
{
	char visited[M->n_row];
	for (int i = 0; i < M->n_row; ++i)
		visited[i] = 0;

	MV_GEN_N(_Def)(rowbuf_mat, rowbuf, 1, M->n_col)
	for (int i = 0; i < M->n_row; ++i) {
		if (visited[i])
			continue;
		if (rowperm[i] == i) {
			/* Simple case of a self-cycle; do nothing. */
			visited[i] = 1;
			continue;
		}

		/* Otherwise we need to shuffle things around. */
		MV_GEN_N(_copy_row)(M, i, &rowbuf_mat, 0);
		int e = i, e_next = rowperm[i];
		while (e_next != i) {
			assert(!visited[e_next]);
			visited[e_next] = 1;
			MV_GEN_N(_copy_row)(M, e_next, M, e);

			/* Advance in the cycle */
			e = e_next;
			e_next = rowperm[e_next];
		}
		visited[i] = 1;
		MV_GEN_N(_copy_row)(&rowbuf_mat, 0, M, e);
	}
}

void MV_GEN_N(_permute_cols)(MV_GEN_TYPE* M, const int* colperm)
{
	char visited[M->n_col];
	for (int i = 0; i < M->n_col; ++i)
		visited[i] = 0;

	MV_GEN_N(_Def)(colbuf_mat, colbuf, M->n_row, 1);
	for (int i = 0; i < M->n_col; ++i) {
		if (visited[i])
			continue;
		if (colperm[i] == i) {
			/* Simple case of a self-cycle; do nothing. */
			visited[i] = 1;
			continue;
		}

		/* Otherwise we need to shuffle things around. */
		MV_GEN_N(_copy_col)(M, i, &colbuf_mat, 0);
		int e = i, e_next = colperm[i];
		while (e_next != i) {
			assert(!visited[e_next]);
			visited[e_next] = 1;
			MV_GEN_N(_copy_col)(M, e_next, M, e);

			/* Advance in the cycle */
			e = e_next;
			e_next = colperm[e_next];
		}
		visited[i] = 1;
		MV_GEN_N(_copy_col)(&colbuf_mat, 0, M, e);
	}
}

void MV_GEN_N(_add)(const MV_GEN_TYPE* A, const MV_GEN_TYPE* B, MV_GEN_TYPE* out)
{
	assert(A->n_row == B->n_row);
	assert(A->n_col == B->n_col);
	assert(A->n_row == out->n_row);
	assert(A->n_col == out->n_col);

	for (int i = 0; i < A->n_row; ++i) {
		MV_GEN_N(_copy_row)(A, i, out, i);
		MV_GEN_N(_multadd_row)(B, i, 1, out, i);
	}
}

void MV_GEN_N(_add_inplace)(const MV_GEN_TYPE* A, MV_GEN_TYPE* B_inout)
{
	assert(A->n_row == B_inout->n_row);
	assert(A->n_col == B_inout->n_col);

	for (int r = 0; r < B_inout->n_row; ++r) {
		MV_GEN_N(_multadd_row)(A, r, 1, B_inout, r);
	}
}

void MV_GEN_N(_mul)(const MV_GEN_TYPE* A, const MV_GEN_TYPE* B, MV_GEN_TYPE* AB_out)
{
	assert(A->n_col == B->n_row);
	assert(A->n_row == AB_out->n_row);
	assert(B->n_col == AB_out->n_col);

	for (int i = 0; i < AB_out->n_row; ++i) {
		for (int j = 0; j < AB_out->n_col; ++j) {
			MV_GEN_ELTYPE x = 0;
			for (int e = 0; e < A->n_col; ++e) {
				const MV_GEN_ELTYPE a = MV_GEN_N(_get_el)(A, i, e);
				const MV_GEN_ELTYPE b = MV_GEN_N(_get_el)(B, e, j);
				x = fadd(x, fmul(a, b));
			}
			MV_GEN_N(_set_el)(AB_out, i, j, x);
		}
	}
}

int MV_GEN_N(_LU_decomp_inplace)(MV_GEN_TYPE* A,
				int* rp,
				int* cp)
{
	/* Initialize permutations */
	int i;
	for (i = 0; i < A->n_row; ++i)
		rp[i] = i;
	for (i = 0; i < A->n_col; ++i)
		cp[i] = i;

	/* LU decomposition */
	for (i = 0; i < A->n_col && i < A->n_row; ++i) {
		/* Find a pivot
		 *
		 * We must pick in A to process that does not lead to a
		 * zero value on the diagonal of U, for otherwise we
		 * can't keep processing.
		 *
		 * Find a row such that the resulting U[i,i]
		 * will be nonzero.
		 */
		int prow;
		int pcol;
		for (pcol = i; pcol < A->n_col; ++pcol) {
			for (prow = i; prow < A->n_row; ++prow) {
				if (MV_GEN_N(_get_el)(A, prow, pcol) != 0) {
					goto pivot_found;
				}
			}
		}

		/* No pivot found; maximum rank reached */
		assert (pcol == A->n_col);
		break;
pivot_found:

		/* Permute row and column to get pivot into place,
		   and record */
		if (prow != i) {
			SwapInt(rp[i], rp[prow]);
			MV_GEN_N(_swap_rows)(A, i, prow);
		}
		if (pcol != i) {
			SwapInt(cp[i], cp[pcol]);
			MV_GEN_N(_swap_cols)(A, pcol, i);
		}

		/* At this point, the row i of U is computed correctly
		 * already, due to the updates to a (see below, "update
		 * A in the undecomposed part").  The column i of L is
		 * not quite correct yet; we need to divide the entries
		 * by Uii.
		 */

		/* Compute the column i of L */
		MV_GEN_ELTYPE Uii_inv = finv(MV_GEN_N(_get_el)(A, i, i));
		MV_GEN_N(_mult_col_from)(A, i, i + 1, Uii_inv);

		/* Update A in the yet undecomposed part */
		for (int j = i + 1; j < A->n_row; ++j) {
			const MV_GEN_ELTYPE Lji = MV_GEN_N(_get_el)(A, j, i);
			MV_GEN_N(_multadd_row_from)(A, i, i + 1, Lji, A, j);
		}
	}

	/* At this point, i is the rank of the matrix */
	return i;
}

MV_GEN_ELTYPE MV_GEN_N(_LU_det)(const MV_GEN_TYPE* LU)
{
	assert(LU->n_row == LU->n_col);

	MV_GEN_ELTYPE det = 1;
	for (int i = 0; i < LU->n_col; ++i) {
		det = fmul(det, MV_GEN_N(_get_el)(LU, i, i));
	}

	return det;
}

void MV_GEN_N(_LU_mult)(const MV_GEN_TYPE* LU,
		   const int* rp,
		   const int* cp,
		   const MV_GEN_TYPE* X,
		   MV_GEN_TYPE* Y_out)
{
	/* Check dimensions */
	assert(LU->n_col == X->n_row);
	assert(LU->n_row == Y_out->n_row);
	assert(X->n_col == Y_out->n_col);

	/* Clear out the rows in the target vector */
	for (int i = 0; i < Y_out->n_row; ++i) {
		MV_GEN_N(_clear_row)(Y_out, i);
	}

	/* Multiply with U */
	for (int i = 0; i < LU->n_row; ++i) {
		const int t = rp[i];
		for (int j = LU->n_col - 1; j >= i; --j) {
			MV_GEN_N(_multadd_row)(X, cp[j],
					MV_GEN_N(_get_el)(LU, i, j),
					Y_out, t);
		}
	}

	/* Multiply with L */
	for (int i = LU->n_row - 1; i >= 0; --i) {
		if (i >= LU->n_col) {
			/* We're in the rectangle below the triangular part
			 */
			for (int j = LU->n_col - 1; j >= 0; --j) {
				MV_GEN_N(_multadd_row)(Y_out, rp[j],
						MV_GEN_N(_get_el)(LU, i, j),
						Y_out, rp[i]);
			}
		} else {
			/* i < LU->n_col, we're in the triangular part */
			for (int j = i - 1; j >= 0; --j) {
				MV_GEN_N(_multadd_row)(Y_out, rp[j],
						MV_GEN_N(_get_el)(LU, i, j),
						Y_out, rp[i]);
			}
		}
	}
}

void MV_GEN_N(_LU_invmult)(const MV_GEN_TYPE* LU,
			int rank,
			const int* rowperm,
			const int* colperm,
			const MV_GEN_TYPE* Y,
			MV_GEN_TYPE* X_out)
{
	/* Check dimensions */
	assert(LU->n_col == X_out->n_row);
	assert(LU->n_row == Y->n_row);
	assert(X_out->n_col == Y->n_col);

	/* If the rank is unspecified (-1), we assume the max rank */
	if (rank == -1)
		rank = (LU->n_col < LU->n_row ? LU->n_col : LU->n_row);

	/* Clear out the unused rows in the target vector */
	for (int i = rank; i < X_out->n_row; ++i) {
		MV_GEN_N(_clear_row)(X_out, colperm[i]);
	}

	/* Multiply with L^-1 */
	for (int i = 0; i < rank; ++i) {
		MV_GEN_N(_copy_row)(Y, rowperm[i], X_out, colperm[i]);
		for (int j = 0; j < i; ++j) {
			MV_GEN_N(_multadd_row)(X_out, colperm[j],
					MV_GEN_N(_get_el)(LU, i, j),
					X_out, colperm[i]);
		}
	}

	/* Multiply with U^-1 */
	for (int i = rank - 1; i >= 0; --i) {
		for (int j = i + 1; j < LU->n_col; ++j) {
			MV_GEN_N(_multadd_row)(X_out, colperm[j],
			  MV_GEN_N(_get_el)(LU, i, j), X_out, colperm[i]);
		}
		MV_GEN_N(_mult_row)(X_out, colperm[i],
				finv(MV_GEN_N(_get_el)(LU, i, i)));
	}
}

void MV_GEN_N(_LU_mult_inplace)(const MV_GEN_TYPE* LU,
		   const int* inv_rowperm,
		   const int* colperm,
		   MV_GEN_TYPE* X_inout)
{
	/* Create the views for inputs & outputs
	 *
	 * The views access the same underlying backend storage,
	 * so some elements alias.  The number of rows of the views
	 * differ, however.
	 */
	assert(LU->n_col <= X_inout->n_row);
	assert(LU->n_row <= X_inout->n_row);
	MV_GEN_TYPE X = MV_GEN_N(_get_subview)(X_inout, 0, 0, LU->n_col, X_inout->n_col);
	MV_GEN_TYPE Y = MV_GEN_N(_get_subview)(X_inout, 0, 0, LU->n_row, X_inout->n_col);

	/* Apply the column permutation */
	if (colperm != NULL) {
		MV_GEN_N(_permute_rows)(&X, colperm);
	}

	MV_GEN_N(_U_mult_inplace)(LU, X_inout);
	MV_GEN_N(_L_mult_inplace)(LU, X_inout);

	/* Apply the row permutation */
	if (inv_rowperm != NULL) {
		MV_GEN_N(_permute_rows)(&Y, inv_rowperm);
	}
}

void MV_GEN_N(_LU_invmult_inplace)(const MV_GEN_TYPE* LU,
				int rank,
				const int* rowperm,
				const int* inv_colperm,
				MV_GEN_TYPE* X_inout)
{
	/* If the rank is unspecified (-1), we assume the max rank */
	if (rank == -1)
		rank = (LU->n_col < LU->n_row ? LU->n_col : LU->n_row);

	/* Create adequate views */
	assert(LU->n_col <= X_inout->n_row);
	assert(LU->n_row <= X_inout->n_row);
	MV_GEN_TYPE X = MV_GEN_N(_get_subview)(X_inout, 0, 0, LU->n_col, X_inout->n_col);
	MV_GEN_TYPE Y = MV_GEN_N(_get_subview)(X_inout, 0, 0, LU->n_row, X_inout->n_col);

	/* Apply the row permutation */
	if (rowperm != NULL) {
		MV_GEN_N(_permute_rows)(&Y, rowperm);
	}

	MV_GEN_N(_L_invmult_inplace)(LU, rank, &Y);
	MV_GEN_N(_U_invmult_inplace)(LU, rank, &X);

	/* Apply the column permutation */
	if (inv_colperm != NULL) {
		MV_GEN_N(_permute_rows)(&X, inv_colperm);
	}
}

void MV_GEN_N(_L_mult_inplace)(const MV_GEN_TYPE* LU, MV_GEN_TYPE* X_inout)
{
	for (int i = LU->n_row - 1; i >= 0; --i) {
		if (i >= LU->n_col) {
			/* We're in the rectangle below the triangular part
			 */
			MV_GEN_N(_clear_row)(X_inout /* Y */, i);
			for (int j = LU->n_col - 1; j >= 0; --j) {
				MV_GEN_N(_multadd_row)(X_inout, j,
						MV_GEN_N(_get_el)(LU, i, j),
						X_inout, i);
			}
		} else {
			/* i < LU->n_col, we're in the triangular part */
			for (int j = i - 1; j >= 0; --j) {
				MV_GEN_N(_multadd_row)(X_inout, j,
						MV_GEN_N(_get_el)(LU, i, j),
						X_inout, i);
			}
		}
	}
}

void MV_GEN_N(_U_mult_inplace)(const MV_GEN_TYPE* LU, MV_GEN_TYPE* X_inout)
{
	for (int i = 0; i < LU->n_col && i < LU->n_row; ++i) {
		MV_GEN_N(_mult_row)(X_inout, i, MV_GEN_N(_get_el)(LU, i, i));
		for (int j = i + 1; j < LU->n_col; ++j) {
			MV_GEN_N(_multadd_row)(X_inout, j,
					MV_GEN_N(_get_el)(LU, i, j),
					X_inout, i);
		}
	}
}

void MV_GEN_N(_L_invmult_inplace)(const MV_GEN_TYPE* LU,
				int rank,
				MV_GEN_TYPE* X_inout)
{
	MV_GEN_N(_L_invmult_inplace_p)(LU, rank, X_inout, NULL);
}

void MV_GEN_N(_U_invmult_inplace)(const MV_GEN_TYPE* LU,
				int rank,
				MV_GEN_TYPE* X_inout)
{
	MV_GEN_N(_U_invmult_inplace_p)(LU, rank, X_inout, NULL);
}

void MV_GEN_N(_L_invmult_inplace_p)(const MV_GEN_TYPE* LU,
				int rank,
				MV_GEN_TYPE* X_inout,
				const int* placements)
{
	/* Apply L^(-1) */
	for (int i = 0; i < rank; ++i) {
		const int pi = (placements ? placements[i] : i);
		for (int j = 0; j < i; ++j) {
			const int pj = (placements ? placements[j] : j);
			MV_GEN_N(_multadd_row)(X_inout, pj,
					MV_GEN_N(_get_el)(LU, i, j),
					X_inout, pi);
		}
	}
}

void MV_GEN_N(_U_invmult_inplace_p)(const MV_GEN_TYPE* LU,
				int rank,
				MV_GEN_TYPE* X_inout,
				const int* placements)
{
	/* Apply U^(-1) */
	for (int i = rank - 1; i >= 0; --i) {
		const int pi = (placements ? placements[i] : i);
		for (int j = i + 1; j < LU->n_col; ++j) {
			const int pj = (placements ? placements[j] : j);
			MV_GEN_N(_multadd_row)(X_inout, pj,
			  MV_GEN_N(_get_el)(LU, i, j),
			  X_inout, pi);
		}
		MV_GEN_N(_mult_row)(X_inout, pi,
				finv(MV_GEN_N(_get_el)(LU, i, i)));
	}
}

#endif /* MV_GEN_DEFINITIONS */

#undef MV_GEN_CAT_
#undef MV_GEN_CAT
#undef MV_GEN_N
