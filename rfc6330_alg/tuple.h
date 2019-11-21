#ifndef TUPLE_H
#define TUPLE_H

#include "parameters.h"

// Sect 5.3.3.2.
typedef struct {
	int d, a, b;
	int d1, a1, b1;
} tuple;

tuple tuple_generate_from_ISI(uint32_t ISI, const parameters* P);
tuple tuple_generate_from_ESI(uint32_t ESI, const parameters* P);

#endif /* TUPLE_H */
