#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "test_utils.h"

/* Compute a set of random, distinct ESIs. */
void get_random_esis(int nESIs, uint32_t* ESIs)
{
	/* We use a linear probing hashing table
	 * to detect collisions.
	 */
	int hashtab_sz = 3*nESIs;
	uint32_t hESIs[hashtab_sz];
	const uint32_t hempty = (uint32_t)-1;
	for (int i = 0; i < hashtab_sz; ++i)
		hESIs[i] = hempty;
	for (int j = 0; j < nESIs; ++j) {
		uint32_t proposed_value;
		bool retry;
		int hidx;
		do {
			proposed_value = rand() & 0xffffff;
			hidx = (int)(proposed_value % hashtab_sz);
			retry = false;
			while (hESIs[hidx] != hempty) {
				if (hESIs[hidx] == proposed_value) {
					retry = true;
					break;
				}
				if (++hidx == hashtab_sz) {
					hidx = 0;
				}
			}
		} while(retry);
		assert(hESIs[hidx] == hempty);
		hESIs[hidx] = proposed_value;
		ESIs[j] = proposed_value;
	}
}

/* Compute the ESIs, taking losses into account.
 *
 * Sect B.1.1 of Amin & Mike's monograph.
 */
void get_esis_after_loss(int nESIs, uint32_t* ESIs, float loss)
{
	int esi = -1;
	for (int j = 0; j < nESIs; ++j) {
		do {
			++esi;
		} while (rand() / (RAND_MAX + 1.0) < loss);
		ESIs[j] = esi;
	}
}
