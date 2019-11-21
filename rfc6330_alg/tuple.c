#include <assert.h>
#include <stdint.h>

#include "rand.h"
#include "tuple.h"

// Sect 5.3.5.2
static const int f[] = {
	0,
	5243,
	529531,
	704294,
	791675,
	844104,
	879057,
	904023,
	922747,
	937311,
	948962,
	958494,
	966438,
	973160,
	978921,
	983914,
	988283,
	992138,
	995565,
	998631,
	1001391,
	1003887,
	1006157,
	1008229,
	1010129,
	1011876,
	1013490,
	1014983,
	1016370,
	1017662,
	1048576,
};

// Sect 5.3.5.2.
static int Deg(int v, int W)
{
	assert(0 <= v);
	assert(v < (1 << 20));

	for (int d = 1; d < sizeof(f)/sizeof(f[0]); ++d) {
		if (f[d - 1] <= v && v < f[d]) {
			return ((d <= W - 2) ? d : (W - 2));
		}
	}
	assert(0); // Should never happen, given the precondition.
	return -1;
}

// Sect 5.3.5.4.
tuple tuple_generate_from_ISI(uint32_t X, const parameters* P)
{
	const uint32_t A = (53591 + P->J * 997) | 0x1;
	const uint32_t B = 10267 * (P->J + 1);
	const uint32_t y = (B + X*A);
	const uint32_t v = Rand(y, 0, 1 << 20);

	tuple T;
	T.d = Deg(v, P->W);
	T.a = 1 + Rand(y, 1, P->W - 1);
	T.b = Rand(y, 2, P->W);
	if (T.d < 4) {
		T.d1 = 2 + Rand(X, 3, 2);
	} else {
		T.d1 = 2;
	}
	T.a1 = 1 + Rand(X, 4, P->P1 - 1);
	T.b1 = Rand(X, 5, P->P1);

	return T;
}

tuple tuple_generate_from_ESI(uint32_t X, const parameters* P)
{
	if (X >= P->K)
		X += P->Kprime - P->K;
	return tuple_generate_from_ISI(X, P);
}
