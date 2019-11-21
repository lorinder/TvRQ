#ifndef PERM_H
#define PERM_H

/**	@file perm.h
 *
 *	Permutations.
 */

void perm_ident(int n, int* arr);
void perm_rand(int n, int* arr);

void perm_invert(int n, const int* orig, int* inverted_out);

#endif /* PERM_H */
