/**	@file test_rq_encdec_match.c
 *
 *	Check that if a source block is encoded, and the symbols with
 *	ESIs v, v+1, ..., v + K - 1 are then decoded again, that the
 *	original data is obtained again.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <getopt.h>

#include "rq_api.h"

int test_consistency_for(int K, int enc_offs)
{
	int status = 0;

	RqInterWorkMem* work = NULL;
	RqInterProgram* program = NULL;
	RqOutWorkMem* outWork = NULL;
	RqOutProgram* outProg = NULL;
#define DONE() \
	do { \
		if (program)	free(program); \
		if (work)	free(work); \
		if (outWork)	free(outWork); \
		if (outProg)	free(outProg); \
		return status; \
	} while(0);
#define RUN_NOFAIL(x) \
	do { \
		int RUN_NOFAIL_err; \
		if ((RUN_NOFAIL_err = (x)) != 0) { \
			fprintf(stderr, "Error:%s:%d:  %s failed: %d\n", \
				__FILE__, __LINE__, #x, RUN_NOFAIL_err); \
			status = RUN_NOFAIL_err; \
			DONE(); \
		} \
	} while(0)

	size_t progSize, workSize, interSymCount;
	RUN_NOFAIL(RqInterGetMemSizes(K,
			0,
			&workSize,
			&progSize,
			&interSymCount));

	/* Create source data */
	const int dwidth = 4;
	uint8_t src[K * dwidth];
	for (int i = 0; i < K * dwidth; ++i) {
		src[i] = rand() & 0xff;
	}

	/* Create encoding program */
	work = malloc(workSize);
	RUN_NOFAIL(RqInterInit(K, 0, work, workSize));
	RUN_NOFAIL(RqInterAddIds(work, 0, K));

	program = malloc(progSize);
	RUN_NOFAIL(RqInterCompile(work, program, progSize));

	/* Create encoding intermediate block */
	uint8_t iblock[interSymCount * dwidth];
	RUN_NOFAIL(RqInterExecute(program,	// program
				  dwidth,		// symbol size
				  src,		// symbol data
				  sizeof(src),   // sizeof symbol data
				  iblock,		// i-block
				  sizeof(iblock))); // size thereof

	/* Compute encoding */
	size_t outWorkSize, outProgSize;
	RUN_NOFAIL(RqOutGetMemSizes(K, &outWorkSize, &outProgSize));
	outWork = malloc(outWorkSize);
	outProg = malloc(outProgSize);
	RUN_NOFAIL(RqOutInit(K, outWork, outWorkSize));
	RUN_NOFAIL(RqOutAddIds(outWork, enc_offs, K));
	RUN_NOFAIL(RqOutCompile(outWork, outProg, outProgSize));

	uint8_t enc[K * dwidth];
	RUN_NOFAIL(RqOutExecute(outProg,
				dwidth,
				iblock,
				enc,
				sizeof(enc)));

	/* Wipe the old work, intermediate block, and program.
	 * This is not strictly necessary, but we want to be absolutely
	 * sure not to use stale data.
	 */
	memset(work, 0, workSize);
	memset(program, 0, progSize);
	memset(iblock, 0, sizeof(iblock));
	memset(outWork, 0, outWorkSize);
	memset(outProg, 0, outProgSize);

	/* Create decoding program */
	RUN_NOFAIL(RqInterInit(K, 0, work, workSize));
	RUN_NOFAIL(RqInterAddIds(work, enc_offs, K));
	int err = RqInterCompile(work, program, progSize);
	if (err == RQ_ERR_INSUFF_IDS) {
		status = err;
		DONE();
	} else if (err != 0) {
		fprintf(stderr, "Error:%s:%d: RqProgramCompile() failed: %d\n",
				__FILE__, __LINE__, err);
		status = err;
		DONE();
	}
	RUN_NOFAIL(RqInterExecute(program,
				  dwidth,
				  enc,
				  sizeof(enc),
				  iblock,
				  sizeof(iblock)));

	/* Compute decoding */
	RUN_NOFAIL(RqOutInit(K, outWork, outWorkSize));
	RUN_NOFAIL(RqOutAddIds(outWork, 0, K));
	RUN_NOFAIL(RqOutCompile(outWork, outProg, outProgSize));
	uint8_t dec[K * dwidth];
	RUN_NOFAIL(RqOutExecute(outProg,
				dwidth,
				iblock,
				dec,
				sizeof(dec)));

	if (memcmp(src, dec, K * dwidth) != 0)
		status = 1;

	DONE();
#undef RUN_NOFAIL
#undef DONE
}

/**	Check that dec(enc(x)) == x  */
static bool test_consistency(int nTestsPerK)
{
	// Picked some K values arbitrarily
	const int Kvals[] = { 5, 50, 75, 93, 103, 143, 198 };
	const int nKvals = sizeof(Kvals)/sizeof(Kvals[0]);
	bool success = true;
	int ndec = 0;

	printf("Testing codec self-consistency (decoding matches original).\n");
	for (int i = 0; i < nKvals; ++i) {
		const int K = Kvals[i];
		printf("  Test for K=%d\n", K);
		for (int j = 0; j < nTestsPerK; ++j) {
			const int offs = K * j;
			int err = test_consistency_for(Kvals[i], offs);
			if (err == 0) {
				++ndec; // success
			} else if (err == 1) {
				fprintf(stderr, "Error:  Decoding does not match source!\n");
				fprintf(stderr, "        K=%d, j=%d.\n", K, j);
				success = false;
			}
		}
	}
	if (success) {
		printf("--> No inconsistencies detected in any decoding.\n");
	}
	printf("--> %d decodings successful, out of %d total.\n",
		ndec, nKvals*nTestsPerK);

	return success;
}

static void usage()
{
	puts(	"RQ API tests.\n"
		"\n"
		"   -h          display this help screen and exit\n"
		"   -i #        number of iterations per K\n"
		"   -s #        set RNG seed\n"
		"\n"
		"Recommended values for tests somewhat more exhaustive than\n"
		"the defaults:  -i 100"
	);
}

int main(int argc, char** argv)
{
	int nTestsPerK = 20;

	int seed = 0;

	/* scan command lines */
	int c;
	while ((c = getopt(argc, argv, "hu:i:s:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'i':
			nTestsPerK = atoi(optarg);
			break;
		case 's':
			seed = atoi(optarg);
			break;
		case '?':
			exit(EXIT_FAILURE);
		};
	}

	/* Run tests */
	int nfail = 0;
	srand(seed < 0 ? time(0) : seed);
#define RUN_TEST(x) \
	do { \
		if (x) { \
			printf("--> pass\n"); \
		} else { \
			printf("--> FAIL\n"); \
			++nfail; \
		} \
	} while (0)
	RUN_TEST(test_consistency(nTestsPerK));
#undef RUN_TEST

	printf("Overall %d tests failed (%s)\n", nfail,
		(nfail ? "FAIL" : "pass"));
	return (nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
