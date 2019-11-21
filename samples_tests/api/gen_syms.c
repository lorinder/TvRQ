#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <strings.h>
#include <getopt.h>
#include <unistd.h>

#include "rq_api.h"

#include "parse_esis.h"

static void usage()
{
	puts(	"Tool to compute an intermediate block\n"
	        "\n"
		"Reads a set of input symbols from stdin and writes a\n"
		"generated intermediate block to stdout\n"
		"\n"
		"   -h       Display help screen and exit\n"
		"\n"
		"   -m <m>   Set operating mode, can be either:\n"
		"             iblock:  generate intermediate block from\n"
		"                      input symbols [default]\n"
		"             iosym:   generate code symbols from\n"
		"                      intermediate block\n"
		"   -K #     Set code dimension\n"
		"   -T #     Set symbol size\n"
		"   -i #,... Set symbol identifiers of symbols\n"
		"              iblock mode: ESIs of input symbols\n"
		"              iosym mode: ESIs of symbols to generate\n"
	);
}

typedef enum {
	OPMODE_IBLOCK,
	OPMODE_IOSYM,
} opmode;

#define RUN_NOFAIL(call, xit_label) \
	RUN_NOFAIL__internal(call, xit_label, RUN_NOFAIL__err)
#define RUN_NOFAIL__internal(call, xit_label, err) \
	do { \
		int err = call; \
		if (err != 0) { \
			fprintf(stderr, "Error:%s:%d: %s returned %d.\n", \
			  __FILE__, __LINE__, #call, err); \
			goto xit_label; \
		} \
	} while(0)

bool gen_iblock(int K, int T, int n_ESIs, uint32_t* ESIs)
{
	bool success = false;

	RqInterWorkMem* wrk = NULL;
	RqInterProgram* prog = NULL;
	void* indata = NULL;
	void* outdata = NULL;

	if (n_ESIs < K) {
		fprintf(stderr, "Error:  Insufficient number of Symbols.\n"
		                "        K=%d, #ESIs=%d\n", K, n_ESIs);
		goto xit;
	}

	int nExtra = n_ESIs - K;
	size_t work_sz, prog_sz, inter_sym_num;
	RUN_NOFAIL(RqInterGetMemSizes(K,
				nExtra,
				&work_sz,
				&prog_sz,
				&inter_sym_num), xit);
	wrk = malloc(work_sz);
	prog = malloc(prog_sz);

	RUN_NOFAIL(RqInterInit(K, nExtra, wrk, work_sz), xit);
	for (int i = 0; i < n_ESIs; ++i)
		RUN_NOFAIL(RqInterAddIds(wrk, ESIs[i], 1), xit);
	RUN_NOFAIL(RqInterCompile(wrk, prog, prog_sz), xit);

	/* Read source data */
	size_t indata_sz = T * (size_t)n_ESIs;
	indata = malloc(indata_sz);
	size_t ret = fread(indata, 1, indata_sz, stdin);
	if (ret != indata_sz) {
		fprintf(stderr, "Error:  fread() returned a short count.\n");
		goto xit;
	}

	/* Generate iblock data */
	size_t outdata_sz = inter_sym_num * T;
	outdata = malloc(outdata_sz);
	RUN_NOFAIL(RqInterExecute(prog, T, indata, indata_sz, outdata, outdata_sz), xit);

	/* Print them to stdout */
	size_t out_written = fwrite(outdata, 1, outdata_sz, stdout);
	if (out_written != outdata_sz) {
		fprintf(stderr, "Error:  fwrite() returned a short count.\n");
		goto xit;
	}

	/* Done */
	success = true;
xit:
	if (outdata)		free(outdata);
	if (indata)		free(indata);
	if (prog)		free(prog);
	if (wrk)		free(wrk);
	return success;
}

bool gen_iosym(int K, int T, int n_ESIs, uint32_t* ESIs)
{
	bool success = false;

	RqOutWorkMem* wrk = NULL;
	RqOutProgram* prog = NULL;
	void* indata = NULL;
	void* outdata = NULL;

	size_t work_sz, prog_sz, inter_sym_num;
	RUN_NOFAIL(RqInterGetMemSizes(K, 0, NULL, NULL, &inter_sym_num), xit);
	RUN_NOFAIL(RqOutGetMemSizes(n_ESIs,
				&work_sz,
				&prog_sz), xit);
	wrk = malloc(work_sz);
	prog = malloc(prog_sz);

	RUN_NOFAIL(RqOutInit(K, wrk, work_sz), xit);
	for (int i = 0; i < n_ESIs; ++i)
		RUN_NOFAIL(RqOutAddIds(wrk, ESIs[i], 1), xit);
	RUN_NOFAIL(RqOutCompile(wrk, prog, prog_sz), xit);

	/* Read source data */
	size_t indata_sz = T * (size_t)inter_sym_num;
	indata = malloc(indata_sz);
	size_t ret = fread(indata, 1, indata_sz, stdin);
	if (ret != indata_sz) {
		fprintf(stderr, "Error:  fread() returned a short count.\n");
		goto xit;
	}

	/* Generate iblock data */
	size_t outdata_sz = T * n_ESIs;
	outdata = malloc(outdata_sz);
	RUN_NOFAIL(RqOutExecute(prog, T, indata, outdata, outdata_sz), xit);

	/* Print them to stdout */
	size_t out_written = fwrite(outdata, 1, outdata_sz, stdout);
	if (out_written != outdata_sz) {
		fprintf(stderr, "Error:  fwrite() returned a short count.\n");
		goto xit;
	}

	success = true;
xit:
	if (outdata)		free(outdata);
	if (indata)		free(indata);
	if (prog)		free(prog);
	if (wrk)		free(wrk);
	return success;
}

int main(int argc, char** argv)
{
	int K = 60;
	int T = 16;
	opmode mode = OPMODE_IBLOCK;

	/* ESI array */
	int n_ESIs = 0;
	int n_max_ESIs = 32;
	uint32_t* ESIs = malloc(n_max_ESIs * sizeof(uint32_t));

	/* Read command line arguments */
	int c;
	while ((c = getopt(argc, argv, "hm:K:T:i:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'm':
			if (strcasecmp(optarg, "iblock") == 0) {
				mode = OPMODE_IBLOCK;
			} else if (strcasecmp(optarg, "iosym") == 0) {
				mode = OPMODE_IOSYM;
			} else {
				fprintf(stderr, "Error: Mode `%s' unknown "
						"(-m).\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'K':
			K = atoi(optarg);
			break;
		case 'T':
			T = atoi(optarg);
			break;
		case 'i': {
			parse_esis(&n_ESIs, &n_max_ESIs, &ESIs, optarg);
			break;
		}
		case '?':
			exit(EXIT_FAILURE);
		};
	}


	/* Check TTY status */
	if (isatty(fileno(stdout))) {
		fprintf(stderr, "Error:  Output is a TTY, refusing output "
		  "since it will garble the screen.\n");
		return EXIT_FAILURE;
	}

	/* Create Inter Program */
	bool success = true;
	switch (mode) {
		case OPMODE_IBLOCK:
			success = gen_iblock(K, T, n_ESIs, ESIs);
			break;
		case OPMODE_IOSYM:
			success = gen_iosym(K, T, n_ESIs, ESIs);
			break;
	};

	/* Finish */
	free(ESIs);
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
