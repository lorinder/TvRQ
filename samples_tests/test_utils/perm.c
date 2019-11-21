#include <stdlib.h>

#include "perm.h"

void perm_ident(int n, int* arr)
{
	for (int i = 0; i < n; ++i)
		arr[i] = i;
}

void perm_rand(int n, int* arr)
{
	perm_ident(n, arr);
	for (int i = 0; i < n - 1; ++i) {
		int j = i + rand() % (n - i);
		const int sw = arr[j];
		arr[j] = arr[i];
		arr[i] = sw;
	}
}

void perm_invert(int n, const int* orig, int* inverted_out)
{
	for (int i = 0; i < n; ++i)
		inverted_out[orig[i]] = i;
}
