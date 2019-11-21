#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "parse_esis.h"

static void append(int* arr_sz,
		int* reserved_sz,
		uint32_t** arr,
		uint32_t val)
{
	if (*arr_sz == *reserved_sz) {
		*reserved_sz *= 2;
		*arr = realloc(*arr,
			*reserved_sz * sizeof(**arr));
	}
	(*arr)[(*arr_sz)++] = val;
}

int parse_esis(int* arr_sz,
		int* reserved_sz,
		uint32_t** esi_arr,
		const char* p)
{
	do {
		char* endptr;
		long int v = strtol(p, &endptr, 0);
		if (endptr == p)
			break;
		p = endptr;
		if (*p == '-') {
			/* A range was given, find the end value and
			 * place the entire range in the array.
			 */
			long int ve = strtol(p + 1, &endptr, 0);
			p = endptr;
			while (v <= ve) {
				append(arr_sz, reserved_sz, esi_arr, v);
				++v;
			}
		} else {
			append(arr_sz, reserved_sz, esi_arr, v);
		}
		while (*p != '\0' && isspace(*p))
			++p;
		if (*p == ',')
			++p;
	} while(1);

	return 0;
}
