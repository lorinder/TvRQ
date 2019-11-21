/**     @file test_perf.cpp
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>
#include <fstream>
#include <sstream>

#include <getopt.h>

#include "rq_api.h"
#include "test_utils.h"

using namespace std;

#define PAGE_SZ 4096

typedef struct {
    clock_t interCompileTime = 0;
    clock_t outCompileTime = 0;
    clock_t totCompileTime = 0;
    clock_t interExecTime = 0;
    clock_t outExecTime = 0;
    clock_t totExecTime = 0;
    int symSize = 0;
    int numSrcBlk = 0;
    int outputEsiCnt = 0;
    int inputEsiCnt = 0;
    int nIter = 0;
    int iterCnt = 0;
} perf_metrics;

void initPerfMetrics (perf_metrics * metrics, int symSize, int numSrcBlk, int outputEsiCnt,
		      int inputEsiCnt, int nIter)
{
    metrics->symSize = symSize;
    metrics->numSrcBlk = numSrcBlk;
    metrics->outputEsiCnt = outputEsiCnt;
    metrics->inputEsiCnt = inputEsiCnt;
    metrics->nIter = nIter;
    return;
}

/* Function to setup src blk */
int setupSrcBlk (int K,
                size_t symSize,
                int inputEsiCnt,
		 uint32_t *inputESIs,
                int numSrcBlk,
                uint8_t* src)
{

    int status = 0;

    RqInterWorkMem*     interWork = NULL;
    RqInterProgram*     interProgram = NULL;
    RqOutWorkMem*       outWork = NULL;
    RqOutProgram*       outProgram = NULL;
    void*               iblock = NULL;

    uint8_t* randsrc = NULL;
#define DONE()                                                  \
    do {                                                        \
        if (iblock)          free(iblock);                      \
        if (interProgram)    free(interProgram);                \
        if (interWork)       free(interWork);                   \
        if (outWork)         free(outWork);                     \
        if (outProgram)      free(outProgram);                  \
        if (randsrc)         free(randsrc);                     \
        if (status == 0) return true; else return false;        \
    } while(0);
#define RUN_NOFAIL(x)   RUN_NOFAIL__INT(x, RUN_NOFAIL__err)
#define RUN_NOFAIL__INT(x,           err)                       \
    do {                                                        \
        int err;                                                \
        if ((err = (x)) != 0) {                                 \
            fprintf(stderr, "Error:%s:%d:  %s failed: %d\n",    \
                    __FILE__, __LINE__, #x, err);               \
            status = err;                                       \
            DONE();                                             \
        }                                                       \
    } while(0)

    randsrc = (uint8_t*)aligned_alloc(PAGE_SZ, K * symSize);

    size_t interWorkSize, interProgSize, iblockSymCnt;
    RUN_NOFAIL(RqInterGetMemSizes(K,
                                  0,
                                  &interWorkSize,
                                  &interProgSize,
                                  &iblockSymCnt));

    /* Create encoding interProgram */
    interWork = (RqInterWorkMem*)malloc(interWorkSize);
    interProgram = (RqInterProgram*)malloc(interProgSize);

    /* Create encoding output symbol Program */
    size_t outWorkSize, outProgSize;
    RUN_NOFAIL(RqOutGetMemSizes(inputEsiCnt, &outWorkSize, &outProgSize));
    outWork = (RqOutWorkMem*)malloc(outWorkSize);
    outProgram = (RqOutProgram*)malloc(outProgSize);

    RUN_NOFAIL(RqInterInit(K, 0, interWork, interWorkSize));
    for (int i = 0; i < K; ++i) {
        RqInterAddIds(interWork, i, 1);
    }
    RUN_NOFAIL(RqInterCompile(interWork, interProgram, interProgSize));

    RUN_NOFAIL(RqOutInit(K, outWork, outWorkSize));
    for (int i = 0; i < inputEsiCnt; ++i)
        RUN_NOFAIL(RqOutAddIds(outWork, inputESIs[i], 1));
    RUN_NOFAIL(RqOutCompile(outWork, outProgram, outProgSize));

    size_t iblock_size = iblockSymCnt * symSize;
    iblock = aligned_alloc(PAGE_SZ, iblockSymCnt * symSize);
    for (int index = 0; index < numSrcBlk; ++index) {

        /* Generate random data */
        for (size_t i = 0; i < K * symSize; i++) {
            randsrc[i] = rand() & 0xff;
        }

        /* Create intermediate block */
        RUN_NOFAIL(RqInterExecute(interProgram,         // interProgram
                                  symSize,              // symbol size
                                  randsrc,              // symbol data
                                  K * symSize,
                                  iblock,               // i-block
                                  iblock_size));         // size thereof



        /* Compute encoding/decoding */
        RUN_NOFAIL(RqOutExecute(outProgram,
                                symSize,
                                iblock,
                                src + index*(inputEsiCnt * symSize),
                                inputEsiCnt * symSize));
    }

    DONE();
#undef RUN_NOFAIL__INT
#undef RUN_NOFAIL
#undef DONE
}

void dumpFinalMetrics (perf_metrics * metrics, int nFail)
{

    printf("\n\nFinal Metrics: Runs %d, nFail = %d\n", metrics->nIter, nFail);

    int nSucc = metrics->nIter - nFail;

    double interCompileTimeSecs = (double)metrics->interCompileTime/CLOCKS_PER_SEC;
    printf ("Inter compile: %ld clicks, %lf seconds\n", metrics->interCompileTime,
            interCompileTimeSecs);

    double outCompileTimeSecs = (double)metrics->outCompileTime/CLOCKS_PER_SEC;
    printf ("Out compile: %ld clicks, %lf seconds\n", metrics->outCompileTime,
            outCompileTimeSecs);

    double totCompileTimeSecs = (double)metrics->totCompileTime/CLOCKS_PER_SEC;
    printf ("Total compile: %ld clicks, %lf seconds\n", metrics->totCompileTime,
            totCompileTimeSecs);

    double interCompileRate = (metrics->inputEsiCnt * metrics->symSize * nSucc)/
	interCompileTimeSecs * 8.0/1000000.0;
    double outCompileRate = (metrics->outputEsiCnt * metrics->symSize * nSucc)/
	outCompileTimeSecs * 8.0/1000000.0;
    double combinedCompileRate = (metrics->inputEsiCnt + metrics->outputEsiCnt) *
	metrics->symSize * nSucc/totCompileTimeSecs * 8.0/1000000.0;

    printf ("Inter Compile Rate: %g Mbps\n", interCompileRate);
    printf ("Out Compile Rate: %g Mbps\n", outCompileRate);
    printf ("Total Compile Rate: %g Mbps\n", combinedCompileRate);

    double totExecTimeSecs = (double)metrics->totExecTime/CLOCKS_PER_SEC;
    printf ("Total Execute: %ld clicks, %lf seconds\n", metrics->totExecTime,
           totExecTimeSecs);

    double interExecTimeSecs = (double)metrics->interExecTime/CLOCKS_PER_SEC;
    printf ("Total Time taken for inter execute: %ld clicks, %lf seconds\n",
	    metrics->interExecTime, interExecTimeSecs);

    double outExecTimeSecs = (double)metrics->outExecTime/CLOCKS_PER_SEC;
    printf ("Total Time taken for out execute: %ld clicks, %lf seconds\n", metrics->outExecTime,
            outExecTimeSecs);

    long inputSize = (long)metrics->numSrcBlk * metrics->inputEsiCnt * metrics->symSize * nSucc;
    long outputSize = (long)metrics->numSrcBlk * metrics->outputEsiCnt * metrics->symSize * nSucc;

    double inputRate = inputSize/interExecTimeSecs * 8.0/1000000.0;
    double outputRate = outputSize/outExecTimeSecs * 8.0/1000000.0;
    double combinedRate = (inputSize + outputSize)/totExecTimeSecs * 8.0/1000000.0;

    printf ("Inter Exec Rate: %g Mbps\n", inputRate);
    printf ("Out Exec Rate: %g Mbps\n", outputRate);
    printf ("Combined Rate: %g Mbps\n", combinedRate);

    return;
}

void dumpCompileTimeMetrics (clock_t startTime, clock_t interCompileFinish,
			     clock_t outCompileFinish, perf_metrics * metrics)
{
    /* Compile Time Metrics */
    clock_t interCompileTime = interCompileFinish - startTime;
    metrics->interCompileTime += interCompileTime;

    clock_t outCompileTime = outCompileFinish - interCompileFinish;
    metrics->outCompileTime += outCompileTime;

    clock_t totCompileTime = outCompileFinish - startTime;
    double totCompileTimeSecs = (double)totCompileTime/CLOCKS_PER_SEC;
    metrics->totCompileTime += totCompileTime;
    double combinedCompileRate = (metrics->inputEsiCnt + metrics->outputEsiCnt) *
	metrics->symSize/totCompileTimeSecs * 8.0/1000000.0;

    printf ("Iter %d: Compile Rate: %g Mbps\n", metrics->iterCnt, combinedCompileRate);

    return;
}

void dumpExecuteTimeMetrics (clock_t elapsedTime, clock_t interExecTime,
			     clock_t outExecTime, perf_metrics * metrics)
{
        /* Execute Time Metrics */
    double elapsedTimeSecs = (double)elapsedTime/CLOCKS_PER_SEC;
    metrics->totExecTime += elapsedTime;
    metrics->interExecTime += interExecTime;
    metrics->outExecTime += outExecTime;

    long inputSize = (long)metrics->numSrcBlk * metrics->inputEsiCnt * metrics->symSize;
    long outputSize = (long)metrics->numSrcBlk * metrics->outputEsiCnt * metrics->symSize;

    double combinedRate = (inputSize + outputSize)/elapsedTimeSecs * 8.0/1000000.0;

    printf ("Iter %d: Execute Rate: %g Mbps\n", metrics->iterCnt, combinedRate);

    return;
}

/* Function to transcode input to output using RaptorQ
*/
int transcode(int K, int symSize, int inputEsiCnt, int outputEsiCnt, int numSrcBlk,
              int offset, int loss, perf_metrics * metrics)
{
    int status = 0;

    RqInterWorkMem* interWork = NULL;
    RqInterProgram* interProgram = NULL;
    RqOutWorkMem*   outWork = NULL;
    RqOutProgram*   outProgram = NULL;

    uint8_t* src = NULL;
    uint8_t* enc = NULL;
    uint8_t* iblock = NULL;
#define DONE()                                                  \
    do {                                                        \
        if (interProgram)    free(interProgram);                \
        if (interWork)       free(interWork);                   \
        if (outWork)         free(outWork);                     \
        if (outProgram)      free(outProgram);                  \
        if (src)             free(src);                     \
        if (enc)             free(enc);                  \
        if (iblock)          free(iblock);			\
        if (status == 0) return true; else return false;        \
    } while(0);
#define RUN_NOFAIL(x)   RUN_NOFAIL__INT(x, RUN_NOFAIL__err)
#define RUN_NOFAIL__INT(x,           err)                       \
    do {                                                        \
        int err;                                                \
        if ((err = (x)) != 0) {                                 \
            fprintf(stderr, "Error:%s:%d:  %s failed: %d\n",    \
                    __FILE__, __LINE__, #x, err);               \
            status = err;                                       \
            DONE();                                             \
        }                                                       \
    } while(0)

    /* Setup inputESI sequence */
    uint32_t inputESIs[inputEsiCnt];
    if (loss == 100) {
	get_random_esis(inputEsiCnt, inputESIs);
    } else {
	get_esis_after_loss(inputEsiCnt, inputESIs, loss/100.0);
    }

    /* Setup src and enc block */
    src = (uint8_t*)aligned_alloc(PAGE_SZ,
                                inputEsiCnt * symSize * numSrcBlk);
    setupSrcBlk(K, symSize, inputEsiCnt, inputESIs, numSrcBlk, src);

    enc = (uint8_t*)aligned_alloc(PAGE_SZ, outputEsiCnt * symSize);

    size_t interWorkSize, interProgSize, iblockSymCnt;

    /* Get various memory sizes from RQ API */
    RUN_NOFAIL(RqInterGetMemSizes(K,
                                inputEsiCnt - K,
                                &interWorkSize,
                                &interProgSize,
                                &iblockSymCnt));

    iblock = (uint8_t*)aligned_alloc(PAGE_SZ,iblockSymCnt * symSize);

    /* Create encoding interProgram */
    interWork = (RqInterWorkMem*)malloc(interWorkSize);
    interProgram = (RqInterProgram*)malloc(interProgSize);

    /* Create encoding output symbol Program */
    size_t outWorkSize, outProgSize;
    RUN_NOFAIL(RqOutGetMemSizes(outputEsiCnt, &outWorkSize, &outProgSize));
    outWork = (RqOutWorkMem*)malloc(outWorkSize);
    outProgram = (RqOutProgram*)malloc(outProgSize);

    RUN_NOFAIL(RqInterInit(K, inputEsiCnt - K, interWork, interWorkSize));
    for (int i = 0; i < inputEsiCnt; ++i) {
        RqInterAddIds(interWork, inputESIs[i], 1);
    }

    RUN_NOFAIL(RqOutInit(K, outWork, outWorkSize));
    for (int i = 0; i < outputEsiCnt; ++i)
        RUN_NOFAIL(RqOutAddIds(outWork, i+offset, 1));

    clock_t startTime = clock();
    RUN_NOFAIL(RqInterCompile(interWork, interProgram, interProgSize));
    clock_t interCompileFinish = clock();
    RUN_NOFAIL(RqOutCompile(outWork, outProgram, outProgSize));
    clock_t outCompileFinish = clock();

    dumpCompileTimeMetrics (startTime, interCompileFinish, outCompileFinish, metrics);

    /* Transcode the data */

    long index = 0;
    clock_t interExecTime = 0, outExecTime = 0;
    startTime = clock();
    while (index < (inputEsiCnt * symSize * numSrcBlk)) {

	clock_t iterStartTime = clock();

        RUN_NOFAIL(RqInterExecute(interProgram,         // interProgram
                                  symSize,            // symbol size
                                  src + index,                // symbol data
                                  inputEsiCnt * symSize,
                                  iblock,              // i-block
                                  sizeof(iblock))); // size thereof

	clock_t iterInterExecTime = clock();

        index += inputEsiCnt * symSize;

        /* Compute encoding/decoding */

        RUN_NOFAIL(RqOutExecute(outProgram,
                                    symSize,
                                    iblock,
                                    enc,
                                    outputEsiCnt * symSize));
	clock_t iterOutExecTime = clock();

	interExecTime += iterInterExecTime - iterStartTime;
	outExecTime += iterOutExecTime - iterInterExecTime;

        /* End of transcode */

    }

    clock_t elapsedTime = clock() - startTime;
    dumpExecuteTimeMetrics (elapsedTime, interExecTime, outExecTime, metrics);

    DONE();
#undef RUN_NOFAIL__INT
#undef RUN_NOFAIL
#undef DONE
}


static void usage()
{
    puts(       "RQ Performance tool.\n"
                "\n"
                "   -h          display this help screen and exit\n"
                "   -K <value>  the k value to use for encode/decode\n"
                "   -T <value>  the symbol size to use for encode/decode\n"
                "   -I <value>  input ESI Cnt\n"
                "   -O <value>  output ESI Cnt\n"
                "   -n <value>  number of source blocks\n"
                "   -s <value>  set RNG seed\n"
                "   -o <value>  encoding offset for ESIs\n"
		"   -l <value>  loss percentage for input esi sequence\n"
                );
    exit(0);
}

int perf_test(int K, int symSize, int inputEsiCnt, int outputEsiCnt, int numSrcBlk,
	      int offset, int loss, int nIter, perf_metrics * metrics)
{
    int nFail = 0;
    for (int i = 1; i <= nIter; ++i) {

	metrics->iterCnt = i;
	if (!transcode(K, symSize, inputEsiCnt, outputEsiCnt, numSrcBlk, offset, loss, metrics)) {
	    nFail++;
	}

	if (i % 10 == 0) {
	    printf("Completed test %d, nFail = %d\n", i, nFail);
	    fflush(stdout);
	}
    }

    return nFail;
}

int main(int argc, char** argv)
{
    int Kval = 500;
    int symSize = 32;
    int inputEsiCnt = 500;
    int outputEsiCnt = 550;
    int numSrcBlk = 10;
    int seed = -1;
    int offset = 500;
    int loss = 100;
    int nIter = 10;
    perf_metrics metrics;

    /* scan command lines */
    int c;
    while ((c = getopt(argc, argv, "hT:K:I:O:n:s:o:l:c:")) != -1) {
        switch (c) {
        case 'h':
            usage();
            break;
        case 'T':
            symSize = atoi(optarg);
            break;
        case 'K':
            Kval = atoi(optarg);
            break;
        case 'I':
            inputEsiCnt = atoi(optarg);
            break;
        case 'O':
            outputEsiCnt = atoi(optarg);
            break;
        case 'n':
            numSrcBlk = atoi(optarg);
            break;
        case 'o':
            offset = atoi(optarg);
            break;
	case 's':
	    seed = atoi(optarg);
	    break;
	case 'l':
	    loss = atoi(optarg);
	    break;
	case 'c':
	    nIter = atoi(optarg);
	    break;
        case '?':
            exit(EXIT_FAILURE);
        };
    }

    /* Dump the input arguments */
    printf ("Settings used: \n");
    printf ("K: %d\n", Kval);
    printf ("Symbol size: %d\n", symSize);
    printf ("InputEsiCnt: %d\n", inputEsiCnt);
    printf ("OutputEsiCnt: %d\n", outputEsiCnt);
    printf ("Number of Src Blocks: %d\n", numSrcBlk);
    printf ("Seed: %d\n", seed);
    printf ("Encoding ESI offset: %d\n", offset);
    printf ("Loss percentage: %d\n", loss);
    printf ("Number of compile iterations: %d\n", nIter);

    if (inputEsiCnt < Kval) {
        fprintf (stderr, "Error: inputEsiCnt cannot be less than K\n");
        return 1;
    }

    srand(seed < 0 ? time(0) : seed);

    initPerfMetrics (&metrics, symSize, numSrcBlk, outputEsiCnt, inputEsiCnt, nIter);

    int nFail = perf_test(Kval, symSize, inputEsiCnt, outputEsiCnt, numSrcBlk,
			  offset, loss, nIter, &metrics);
    dumpFinalMetrics(&metrics, nFail);

    return EXIT_SUCCESS;
}

// vim:et:ts=8
