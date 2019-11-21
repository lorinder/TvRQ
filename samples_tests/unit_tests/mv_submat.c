#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "m256v.h"
#include "m2v.h"
#include "perm.h"

#include "utils.h"
#include "m2v_m256v_mat_pair.h"

int main(int argc, char** argv)
{
	m2v_Def(A, a, 10, 10)
	for (int i = 0; i < 10; ++i) {
		a[i] = rand();
	}
	puts("The A matrix:");
	print_m2v(stdout, "  ", &A);

	m2v_Def(B, b, 10, 1);
	m2v_copy_submat(&A, 0, 9, 10, 1, &B, 0, 0);
	puts("The B matrix:");
	print_m2v(stdout, "  ", &B);

	return 0;
}
