#ifndef UTILS_H
#define UTILS_H

/**	@file utils.h
 *
 *	Various utility functions and macros
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <limits.h>

#include "m2v.h"
#include "m256v.h"

#define UNUSED		__attribute__((unused))

UNUSED
static uint8_t get_mask(int field)
{
	switch(field) {
	case 2:		return 0x1;	break;
	case 256:	return 0xff;	break;
	default:
		fprintf(stderr, "%s:%d: Unsupported field size %d.\n",
			__FILE__, __LINE__, field);
		exit(EXIT_FAILURE);
	}
	return 0; // Not reached.
}

UNUSED
static void rand_coeffs(uint8_t* m, int N, uint8_t mask)
{
	for (int i = 0; i < N; ++i) {
		m[i] = rand() & mask;
	}
}

UNUSED
static void print_int_arr(FILE* out, int N, const int* arr)
{
	for (int i = 0; i < N; ++i) {
		fprintf(out, "%d%s",
			arr[i], (i < N - 1 ? ", " : "\n"));
	}
}

UNUSED
static void print_mat(FILE* out, const char* indent, const m256v* M)
{
	for (int r = 0; r < M->n_row; ++r) {
		fprintf(out, "%s%s", indent, (r == 0 ? "[" : " "));
		for (int c = 0; c < M->n_col; ++c) {
			fprintf(out, " %02x", (int)m256v_get_el(M, r, c));
		}
		fprintf(out, "%s\n", ((r == M->n_row - 1) ? " ]" : ""));
	}
}

UNUSED
static void print_m2v(FILE* out, const char* indent, const m2v* M)
{
	for (int r = 0; r < M->n_row; ++r) {
		fprintf(out, "%s%s", indent, (r == 0 ? "[" : " "));
		for (int c = 0; c < M->n_col; ++c) {
			fprintf(out, "%d", (int)m2v_get_el(M, r, c));
		}
		fprintf(out, "%s\n", ((r == M->n_row - 1) ? " ]" : ""));
	}
}

#define array_size(x)		(sizeof(x) / sizeof((x)[0]))
#define rand_arr(arr, mask)	rand_coeffs((arr), array_size(arr), (mask))

/* Creation of stack-backed GF(256) matrices */

#define Def_mat256_init(mat_name, store_name, nrow, ncol, ...) \
   uint8_t store_name[(nrow) * (ncol)] = { __VA_ARGS__ }; \
   m256v mat_name = m256v_make((nrow), (ncol), store_name);

#define Def_mat256_rand(mat_name, store_name, nrow, ncol, mask) \
   uint8_t store_name[(nrow) * (ncol)]; \
   rand_arr(store_name, mask); \
   m256v mat_name = m256v_make((nrow), (ncol), store_name);

/* Creation of stack-backed GF(2) matrices */

#define Init_mat2(mat_name, ...) \
	  Init__mat2(mat_name, Init_mat2__i, Init_mat2__arr, __VA_ARGS__)
#define Init__mat2(M,	i, arr, ...) \
   do { \
	   const int arr[] = { __VA_ARGS__ }; \
	   for (int i = 0; i < array_size(arr); ++i) \
		m2v_set_el(&(M), i / (M).n_col, i % (M).n_col, arr[i]); \
   } while(0)

#endif /* UTILS_H */
