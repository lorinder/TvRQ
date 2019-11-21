#ifndef PARSE_ESIS_H
#define PARSE_ESIS_H

#include <inttypes.h>

int parse_esis(int* arr_sz,
		int* reserved_sz,
		uint32_t** esi_arr,
		const char* str);

#endif /* PARSE_ESIS_H */
