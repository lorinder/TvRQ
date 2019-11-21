#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gf256.h"

static bool test_add()
{
	if (gf256_add(1,1) != 0
		|| gf256_add(133, 133) != 0
		|| gf256_add(53, 123) != 78)
	{
		return false;
	}
	return true;
}

static bool test_logexp()
{
	bool success = true;

	/* Walk log table */
	int i;
	for (i = 1; i < 256; ++i) {
		const int l = gf256_log(i);
		if (l < 0 || l > 255) {
			fprintf(stderr, "  gf256_log(%d): got %d\n", i, l);
			success = false;
		}
	}

	/* Walk exp table */
	for (int i = 0; i < 255; ++i) {
		const uint8_t e = gf256_exp(i);
		if (e == 0) {
			fprintf(stderr, "  gf256_exp(%d) == 0 !\n", i);
			success = false;
		}
	}

	/* Check exp(log(x)) == x for x != 0 */
	for (int i = 1; i <= 255; ++i) {
		uint8_t v = gf256_exp(gf256_log(i));
		if (v != i) {
			fprintf(stderr, "  gf256_exp(gf256_log(%d)) == %d !\n",
				i, v);
			success = false;
		}
	}

	return success;
}

static bool test_mul()
{
	bool success = true;

	/* Multiplication against 0 checks */
	if (gf256_mul(0, 0) != 0
	  || gf256_mul(26, 0) != 0
	  || gf256_mul(0, 143) != 0)
	{
		fprintf(stderr, "  nonzero result from 0*x or x*0.\n");
		success = false;
	}

	/* 1 * x and x * 1 */
	for (int i = 0; i <= 255; ++i) {
		const int res1 = gf256_mul(1, i);
		const int res2 = gf256_mul(i, 1);
		if (res1 != i) {
			fprintf(stderr, "  1 * %d == %d !\n", i, res1);
			success = false;
		}
		if (res2 != i) {
			fprintf(stderr, "  1 * %d == %d !\n", i, res2);
			success = false;
		}
	}

	/* Check that x**256 == x */
	for (int i = 0; i <= 255; ++i) {
		uint8_t r = i;
		for (int e = 0; e < 8; ++e) {
			r = gf256_mul(r, r);
		}
		if (r != i) {
			fprintf(stderr, "  %d^256 == %d\n", i, (int)r);
			success = false;
		}
	}

	/* Multiplication by a nonzero constant is bijective */
	{
		char seenl[256] = { 0 };
		char seenr[256] = { 0 };
		for (int i = 0; i < 256; ++i) {
			const uint8_t rl = gf256_mul(57, i);
			const uint8_t rr = gf256_mul(i, 185);
			if (seenl[rl] || seenr[rr]) {
				fprintf(stderr,
				  " x |-> cst * x is not one-on-one\n");
				success = false;
			}
			seenl[rl] = 1;
			seenr[rr] = 1;
		}
	}

	/* Multiplication is commutative */
	for (int i = 0; i < 100; ++i) {
		uint8_t a = rand();
		uint8_t b = rand();
		if (gf256_mul(a, b) != gf256_mul(b, a)) {
			fprintf(stderr, "  %d * %d = %d, but %d * %d = %d\n",
			  a, b, gf256_mul(a, b), b, a, gf256_mul(b, a));
			success = false;
		}
	}

	/* Multiplication is associative */
	for (int i = 0; i < 100; ++i) {
		const uint8_t a = rand();
		const uint8_t b = rand();
		const uint8_t c = rand();

		const uint8_t leftm = gf256_mul(gf256_mul(a, b), c);
		const uint8_t rightm = gf256_mul(a, gf256_mul(b, c));

		if (leftm != rightm) {
			fprintf(stderr, "  (%d * %d) * %d == %d, "
			   "but %d * (%d * %d) == %d\n",
			   a, b, c, leftm,
			   a, b, c, rightm);
			success = false;
		}
	}

	/* Multiplication is distributive */
	for (int i = 0; i < 100; ++i) {
		const uint8_t a = rand();
		const uint8_t b = rand();
		const uint8_t c = rand();

		const uint8_t m1 = gf256_mul(a, gf256_add(b, c));
		const uint8_t m2 = gf256_add(gf256_mul(a, b), gf256_mul(a, c));
		if (m1 != m2) {
			fprintf(stderr, "  %d * (%d + %d) = %d, but "
			  "%d * %d + %d * %d = %d\n",
			  a, b, c, m1,
			  a, b, a, c, m2);
			success = false;
		}
	}

	return success;
}

static bool test_inv()
{
	bool success = true;

	/* x * ~x = 1 */
	for (int i = 1; i <= 255; ++i) {
		const uint8_t result = gf256_mul(i, gf256_inv(i));
		if (result != 1) {
			fprintf(stderr, "  %d * ~%d == %d\n",
				i, i, result);
			success = false;
		}
	}

	/* ~~x = x */
	for (int i = 1; i <= 255; ++i) {
		const uint8_t result = gf256_inv(gf256_inv(i));
		if (result != i) {
			fprintf(stderr, "  ~~%d = %d.\n", i, result);
			success = false;
		}
	}

	return success;
}

int main()
{
	int nfail = 0;
#define RUN_TEST(x) \
	do { \
		fprintf(stderr, "Test: " #x "\n"); \
		if (!x()) { \
			fprintf(stderr, "--> FAIL (test " #x ")\n"); \
			++nfail; \
		} else { \
			fprintf(stderr, "--> pass\n"); \
		} \
	} while(0)

	RUN_TEST(test_add);
	RUN_TEST(test_logexp);
	RUN_TEST(test_mul);
	RUN_TEST(test_inv);

#undef RUN_TEST
	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
