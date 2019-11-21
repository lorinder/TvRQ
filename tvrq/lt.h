#ifndef LT_H
#define LT_H

/**	@file lt.h
 *
 *	Create the LT matrix
 */

#include <stdint.h>

#include "parameters.h"
#include "m256v.h"

void lt_generate_mat(m256v* M,
			const parameters* P,
			int n_ISIs,
			const uint32_t* ISIs);

#endif /* LT_H */
