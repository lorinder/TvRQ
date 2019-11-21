/**	@file test_rq_failprob.c
 *
 *	Compute the failure probability for a given K at 0 symbols overhead.
 *
 *	This is more useful as an interactive test, rather than for the
 *	test suite, as there is no hard failure criterion, only
 *	probabilistic ones.
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
#include "test_utils.h"

int getProgram(int K,
		int nESIs,
		uint32_t* ESIs,
		RqInterProgram** ppProgram)
{
	*ppProgram = NULL;

	RqInterWorkMem* work = NULL;
	RqInterProgram* prog = NULL;
#define RUN_NOFAIL(x) \
	do { \
		int RUN_NOFAIL_err = (x); \
		if (RUN_NOFAIL_err < 0) { \
			fprintf(stderr, "Error:%s:%d: %s failed: %d\n", \
				__FILE__, __LINE__, #x, RUN_NOFAIL_err); \
			if (work) free(work); \
			if (prog) free(prog); \
			return RUN_NOFAIL_err; \
		} \
	} while(0)

	size_t workSize, progSize;
	RUN_NOFAIL(RqInterGetMemSizes(K,
				nESIs - K,
				&workSize,
				&progSize,
				NULL));
	work = malloc(workSize);
	RUN_NOFAIL(RqInterInit(K, nESIs - K, work, workSize));
	for (int i = 0; i < nESIs; ++i) {
		RUN_NOFAIL(RqInterAddIds(work, ESIs[i], 1));
	}
#undef RUN_NOFAIL

	prog = malloc(progSize);
	int err = RqInterCompile(work, prog, progSize);

	/* Free temp mem */
	free(work);

	if (err == 0) {
		*ppProgram = prog;
	} else {
		free(prog);
	}

	return err;
}


/**	Compute decoder performance at 0 overhead.
 *
 *	@param	K
 *		code K value to test.
 *
 *	@param	nTest
 *		Number of iterations to run.
 *
 *	@param	loss
 *		loss model to use; if < 1, a corresponding fixed
 *		fraction of symbols is lost.  If loss = 1, pick ESIs at
 *		random.
 */
int zero_overhead_test(int K, int nTest, float loss)
{
#define RUN_NOFAIL(x) \
	do { \
		int RUN_NOFAIL_err; \
		if ((RUN_NOFAIL_err = (x)) != 0) { \
			fprintf(stderr, "Error:%s:%d:  %s failed: %d\n", \
				__FILE__, __LINE__, #x, RUN_NOFAIL_err); \
			if (config) \
				free(config); \
			return -1; \
		} \
	} while(0)

	int nFail = 0;
	for (int i = 0; i < nTest; ++i) {
		uint32_t ESIs[K];
		if (loss == 1) {
			get_random_esis(K, ESIs);
		} else {
			get_esis_after_loss(K, ESIs, loss);
		}

		/* Create the program */
		RqInterProgram* program = NULL;
		int err = getProgram(K, K, ESIs, &program);
		switch (err) {
		case 0:						break;
		case RQ_ERR_INSUFF_IDS:	++nFail;	break;
		default:
			return -1;
		};

		if (i % 1000 == 0) {
			printf("   Completed test %d, nFail=%d\n", i, nFail);
			fflush(stdout);
		}

		free(program);
	}

#undef RUN_NOFAIL
	return nFail;
}

bool test_fprob(int K, int nTest)
{
	bool success = true;

	/* Tests at fixed losses */
	float losses[] = { 0.1, 0.85, 1 };
	float expected_fps[3] = { -1, -1, -1 };
	if (K == 100) {
		/* Graphs in sect B.3.1 of the monograph were used to
		 * read off expected failure probabilities.
		 */
		expected_fps[0] = pow(10, -2.4); // from B.3.1
		expected_fps[1] = pow(10, -2.3); // from B.3.1
	}
	const int nLosses = (int)(sizeof(losses) / sizeof(losses[0]));
	for (int i = 0; i < nLosses; ++i) {
		const float loss = losses[i];
		const float expected_fp = expected_fps[i];
		printf("Running fixed loss tests. K=%d, nTest=%d, "
		  "loss=%g.\n", K, nTest, (double)loss);
		int nfail = zero_overhead_test(K, nTest, loss);
		if (nfail < 0) {
			fprintf(stderr, "Error:  random_esi_test() failed.\n");
			success = false;
		}
		printf("--> %d failures out of %d runs.\n", nfail, nTest);
		if (expected_fp != -1) {
			printf("    expected would be ~%d.\n",
				(int)round(expected_fp * nTest));
		}
		fflush(stdout);
	}

	return success;
}


static void usage()
{
	puts(	"RQ API tests.\n"
		"\n"
		"   -h          display this help screen and exit\n"
		"   -k #        K-value to test\n"
		"   -i #        number of iterations\n"
		"   -s #        set RNG seed\n"
		"\n"
		"Recommended values for tests somewhat more exhaustive than\n"
		"the defaults:  -i 10000\n"
	);
}

int main(int argc, char** argv)
{
	int K = 100;
	int nIter = 500;
	int seed = 0;

	/* scan command lines */
	int c;
	while ((c = getopt(argc, argv, "hk:i:s:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'k':
			K = atoi(optarg);
			break;
		case 'i':
			nIter = atoi(optarg);
			break;
		case 's':
			seed = atoi(optarg);
			break;
		case '?':
			exit(EXIT_FAILURE);
		};
	}

	/* Run tests */
	srand(seed < 0 ? time(0) : seed);
	if (!test_fprob(K, nIter))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
