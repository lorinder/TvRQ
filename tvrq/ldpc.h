#ifndef LDPC_H
#define LDPC_H

/**	@file ldpc.h
 *
 *	Create the LDPC matrix
 */

#include "parameters.h"
#include "m256v.h"

void ldpc_generate_mat(m256v* L, const parameters* P);

#endif /* LDPC_H */
