#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "m256v.h"
#include "parameters.h"
#include "rq_api.h"
#include "rq_matrix.h"
#include "tuple.h"

#define errmsg(x)	fprintf(stderr, "Error:%s:%d: %s\n", \
				__FILE__, __LINE__, (x))

struct RqInterWorkMem_ {
	parameters params;
	int nESI_max;
	int nESI;
	uint32_t ESIs[];
};

struct RqInterProgram_ {
	m256v LU;
	uint8_t* lu_storage;
	int nESI;
	int rowperm[];
};

struct RqOutWorkMem_ {
	parameters params;
	int nESI_max;
	int nESI;
	uint32_t ESIs[];
};

struct RqOutProgram_ {
	parameters params;
	int unused; // To keep RqOutProgram_ identical to RqOutWorkMem_
	int nESI;
	uint32_t ESIs[];
};

int RqInterGetMemSizes(int nMaxK,
		       int nMaxExtra,
		       size_t* pInterWorkMemSize,
		       size_t* pInterProgMemSize,
		       size_t* pInterSymNum)
{
	/* Compute scheduler size */
	parameters params = parameters_get(nMaxK);
	if (params.K == -1) {
		errmsg("Unsupported K value.");
		return RQ_ERR_EDOM;
	}
	const int maxISIcount = nMaxExtra + params.Kprime;
	if (pInterWorkMemSize != NULL) {
		*pInterWorkMemSize = sizeof(RqInterWorkMem)
					+ maxISIcount * sizeof(uint32_t);
	}

	/* Compute the program size */
	int n_rows = maxISIcount + params.S + params.H;
	int n_cols = params.L;
	if (pInterProgMemSize != NULL) {
		*pInterProgMemSize =
		  sizeof(RqInterProgram)
		  + sizeof(int) * n_rows
		  + n_rows * n_cols;
	}

	/* Intermediate Block Size */
	if (pInterSymNum != NULL) {
		*pInterSymNum = params.L;
	}

	return 0;
}

int RqInterInit(int nK,
		   int nMaxExtra,
		   RqInterWorkMem* pInterWorkMem,
		   size_t nInterWorkMemSize)
{
	if (nInterWorkMemSize < sizeof(RqInterWorkMem)) {
		errmsg("Insufficient inter work memory size.");
		return RQ_ERR_ENOMEM;
	}
	pInterWorkMem->params = parameters_get(nK);
	if (pInterWorkMem->params.K == -1) {
		errmsg("Unsupported K value.");
		return RQ_ERR_EDOM;
	}
	assert(pInterWorkMem->params.K == nK);
	pInterWorkMem->nESI_max
	  = (nInterWorkMemSize - sizeof(RqInterWorkMem)) / sizeof(uint32_t);
	if (pInterWorkMem->nESI_max < nK + nMaxExtra) {
		errmsg("Insufficient inter work memory size.");
		return RQ_ERR_ENOMEM;
	}

	pInterWorkMem->nESI = 0;
	return 0;
}

int RqInterAddIds(RqInterWorkMem* pInterWorkMem,
		  int32_t nInSymIdBeg,
		  int32_t nInSymIdAdd)
{
	int ret = 0;
	if (pInterWorkMem->nESI + nInSymIdAdd > pInterWorkMem->nESI_max) {
		errmsg("Maximum number of ESIs reached.");

		nInSymIdAdd = pInterWorkMem->nESI_max - pInterWorkMem->nESI;
		ret = RQ_ERR_MAX_IDS_REACHED;
	}

	for (int i = 0; i < nInSymIdAdd; ++i) {
		pInterWorkMem->ESIs[pInterWorkMem->nESI++] = nInSymIdBeg + i;
	}

	return ret;
}

int RqInterCompile(RqInterWorkMem* pInterWorkMem,
		   RqInterProgram* pInterProgMem,
		   size_t nInterProgMemSize)
{
	const int n_cols = pInterWorkMem->params.L;
	const int n_rows = pInterWorkMem->nESI	// added LT symbols
		+ pInterWorkMem->params.Kprime - pInterWorkMem->params.K // padding
		+ pInterWorkMem->params.S		// LDPC symbols
		+ pInterWorkMem->params.H;		// HDPC symbols

	/* Mem check */
	const size_t mem_needed = sizeof(RqInterProgram)
					+ sizeof(int) * n_rows
					+ sizeof(uint8_t) * n_rows * n_cols;
	if (nInterProgMemSize < mem_needed) {
		errmsg("Not enough memory for Program.");
		return RQ_ERR_ENOMEM;
	}

	/* Set up fields in the program */
	pInterProgMem->lu_storage = (uint8_t*)((char*)pInterProgMem
						+ sizeof(RqInterProgram)
						+ sizeof(int) * n_rows);
	pInterProgMem->nESI = pInterWorkMem->nESI;
	pInterProgMem->LU = m256v_make(n_rows, n_cols, pInterProgMem->lu_storage);

	/* Create the RQ matrix & LU decompose*/
	rq_matrix_generate(&pInterProgMem->LU,
			&pInterWorkMem->params,
			pInterWorkMem->nESI,
			pInterWorkMem->ESIs);
	int colperm[n_cols];
	const int rank = m256v_LU_decomp_inplace(
				&pInterProgMem->LU,
				pInterProgMem->rowperm,
				colperm);
	if (rank < n_cols) {
		return RQ_ERR_INSUFF_IDS;
	}
	/* We're depending on a property of our LU decomposition here,
	 * namely that when the matrix has rank n_cols, then it does not
	 * permute any columns.  Of course, other pivoting strategies
	 * could behave differently.
	 *
	 * The below sequence of assert() statements verifies this
	 * property.
	 */
	for (int i = 0; i < n_cols; ++i) {
		assert(colperm[i] == i);
	}

	/* Remove unused rows from LU matrix */
	pInterProgMem->LU.n_row = rank;
	assert(pInterProgMem->LU.n_row == pInterProgMem->LU.n_col);

	return 0;
}

int RqInterExecute(const RqInterProgram* pcInterProgMem,
		   size_t nSymSize,
		   const void* pcInSymMem,
		   size_t nInSymMemSize,
		   void* pInterSymMem,
		   size_t nInterSymMemSize)
{
	/* Check memory sizes */
#if 0
	if (symbolDataSize < pcInterProgMem->nESI * nSymSize) {
		errmsg("Too little symbol data provided.");
		return RQ_ERR_ENOMEM;
	}
#endif
	if (nInterSymMemSize < pcInterProgMem->LU.n_col * nSymSize) {
		errmsg("Not enough space for Intermediate block provided.");
		return RQ_ERR_ENOMEM;
	}

	/* Create matrices */
	m256v IB = m256v_make(pcInterProgMem->LU.n_col, nSymSize, pInterSymMem);
	m256v Y = m256v_make(pcInterProgMem->nESI, nSymSize, (void*)pcInSymMem);

	/* Move data into the IB matrix */
	for (int i = 0; i < pcInterProgMem->LU.n_row; ++i) {
		const int l = pcInterProgMem->rowperm[i];
		if (l >= pcInterProgMem->nESI) {
			/* This row contains either a padded LT symbol
			 * or an LDPC symbol or an HDPC symbol.  In
			 * any case the corresponding RHS term is zero
			 */
			m256v_clear_row(&IB, i);
		} else {
			/* Row comes from an LT symbol */
			m256v_copy_row(&Y, l, &IB, i);
		}
	}

	/* Solve */
	m256v_LU_invmult_inplace(&pcInterProgMem->LU, -1, NULL, NULL, &IB);
	return 0;
}

int RqOutGetMemSizes(int nOutSymNum,
		     size_t* pOutWorkMemSize,
		     size_t* pOutProgMemSize)
{
	_Static_assert(sizeof(RqOutWorkMem) == sizeof(RqOutProgram),
			"RqOutWorkMem and RqOutProgram need to be defined identically.");
	const size_t sz = sizeof(RqOutWorkMem) + nOutSymNum*sizeof(uint32_t);
	if (pOutWorkMemSize != NULL) {
		*pOutWorkMemSize = sz;
	}
	if (pOutProgMemSize != NULL) {
		*pOutProgMemSize = sz;
	}
	return 0;
}

int RqOutInit(int nK,
	      RqOutWorkMem* pOutWorkMem,
	      size_t nOutWorkMemSize)
{
	if (sizeof(*pOutWorkMem) > nOutWorkMemSize) {
		errmsg("Not enough memory for OutWorkMem.");
		return RQ_ERR_ENOMEM;
	}

	pOutWorkMem->params = parameters_get(nK);
	if (pOutWorkMem->params.K == -1) {
		errmsg("Unsupported K value.");
		return RQ_ERR_EDOM;
	}

	const int nESImax = (nOutWorkMemSize - sizeof(*pOutWorkMem))
				/ sizeof(pOutWorkMem->ESIs[0]);
	pOutWorkMem->nESI_max = nESImax;
	pOutWorkMem->nESI = 0;

	return 0;
}

int RqOutAddIds(RqOutWorkMem* pOutWorkMem,
		int32_t nOutSymIdBeg,
		int32_t nOutSymIdAdd)
{
	int ret = 0;
	if (pOutWorkMem->nESI + nOutSymIdAdd > pOutWorkMem->nESI_max) {
		errmsg("Maximum number of ESIs reached.");

		nOutSymIdAdd = pOutWorkMem->nESI_max - pOutWorkMem->nESI;
		ret = RQ_ERR_MAX_IDS_REACHED;
	}

	for (int i = 0; i < nOutSymIdAdd; ++i) {
		pOutWorkMem->ESIs[pOutWorkMem->nESI++] = nOutSymIdBeg + i;
	}

	return ret;
}

int RqOutCompile(RqOutWorkMem* pOutWorkMem,
		 RqOutProgram* pOutProgMem,
		 size_t nOutProgMemSize)
{
	size_t sz = sizeof(*pOutProgMem) + pOutWorkMem->nESI * sizeof(pOutProgMem->ESIs[0]);
	if (sz < nOutProgMemSize) {
		errmsg("Not enough memory for OutProgMem.");
		return RQ_ERR_ENOMEM;
	}

	memcpy(pOutProgMem, pOutWorkMem, sz);
	return 0;
}

int RqOutExecute(const RqOutProgram* pcOutProgMem,
		 size_t nSymSize,
		 const void* pcInterSymMem,
		 void* pOutSymMem,
		 size_t nOutSymMemSize)
{
	/* Check memory limitations */
	if (nOutSymMemSize < pcOutProgMem->nESI * nSymSize) {
		errmsg("Not enough space for generated symbols.");
		return RQ_ERR_ENOMEM;
	}

	/* Create matrices */
	const parameters* P = &pcOutProgMem->params;
	m256v O = m256v_make(pcOutProgMem->nESI, nSymSize, pOutSymMem);
	m256v I = m256v_make(P->L, nSymSize, (void*)pcInterSymMem);

	/* Generate symbols */
	for (int i = 0; i < pcOutProgMem->nESI; ++i) {
		// Sect 5.3.5.3
		tuple T = tuple_generate_from_ESI(
				pcOutProgMem->ESIs[i], P);
		m256v_copy_row(&I, T.b, &O, i);
		for (int j = 1; j < T.d; ++j) {
			T.b = (T.b + T.a) % P->W;
			m256v_multadd_row(&I, T.b, 1, &O, i);
		}
		while (T.b1 >= P->P)
			T.b1 = (T.b1 + T.a1) % P->P1;
		m256v_multadd_row(&I, P->W + T.b1, 1, &O, i);
		for (int j = 1; j < T.d1; ++j) {
			do {
				T.b1 = (T.b1 + T.a1) % P->P1;
			} while(T.b1 >= P->P);
			m256v_multadd_row(&I, P->W + T.b1, 1, &O, i);
		}
	}

	return 0;
}
