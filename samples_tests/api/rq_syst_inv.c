/**	@file test_rq_syst_inv.c
 *
 *	For different values of K, check whether the intermediate block
 *	is fully defined from the K source symbols. This is the case for
 *	RaptorQ by design, the systematic indices are chosen that way
 *	(RFC 6330 section 5.6).
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <getopt.h>

#include "rq_api.h"

static const int Kprime[] = {
10, 12, 18, 20, 26, 30, 32, 36, 42, 46, 48, 49, 55, 60, 62, 69, 75, 84, 88,
91, 95, 97, 101, 114, 119, 125, 127, 138, 140, 149, 153, 160, 166, 168, 179,
181, 185, 187, 200, 213, 217, 225, 236, 242, 248, 257, 263, 269, 280, 295,
301, 305, 324, 337, 341, 347, 355, 362, 368, 372, 380, 385, 393, 405, 418,
428, 434, 447, 453, 466, 478, 486, 491, 497, 511, 526, 532, 542, 549, 557,
563, 573, 580, 588, 594, 600, 606, 619, 633, 640, 648, 666, 675, 685, 693,
703, 718, 728, 736, 747, 759, 778, 792, 802, 811, 821, 835, 845, 860, 870,
891, 903, 913, 926, 938, 950, 963, 977, 989, 1002, 1020, 1032, 1050, 1074,
1085, 1099, 1111, 1136, 1152, 1169, 1183, 1205, 1220, 1236, 1255, 1269,
1285, 1306, 1347, 1361, 1389, 1404, 1420, 1436, 1461, 1477, 1502, 1522,
1539, 1561, 1579, 1600, 1616, 1649, 1673, 1698, 1716, 1734, 1759, 1777,
1800, 1824, 1844, 1863, 1887, 1906, 1926, 1954, 1979, 2005, 2040, 2070,
2103, 2125, 2152, 2195, 2217, 2247, 2278, 2315, 2339, 2367, 2392, 2416,
2447, 2473, 2502, 2528, 2565, 2601, 2640, 2668, 2701, 2737, 2772, 2802,
2831, 2875, 2906, 2938, 2979, 3015, 3056, 3101, 3151, 3186, 3224, 3265,
3299, 3344, 3387, 3423, 3466, 3502, 3539, 3579, 3616, 3658, 3697, 3751,
3792, 3840, 3883, 3924, 3970, 4015, 4069, 4112, 4165, 4207, 4252, 4318,
4365, 4418, 4468, 4513, 4567, 4626, 4681, 4731, 4780, 4838, 4901, 4954,
5008, 5063, 5116, 5172, 5225, 5279, 5334, 5391, 5449, 5506, 5566, 5637,
5694, 5763, 5823, 5896, 5975, 6039, 6102, 6169, 6233, 6296, 6363, 6427,
6518, 6589, 6655, 6730, 6799, 6878, 6956, 7033, 7108, 7185, 7281, 7360,
7445, 7520, 7596, 7675, 7770, 7855, 7935, 8030, 8111, 8194, 8290, 8377,
8474, 8559, 8654, 8744, 8837, 8928, 9019, 9111, 9206, 9303, 9400, 9497,
9601, 9708, 9813, 9916, 10017, 10120, 10241, 10351, 10458, 10567, 10676,
10787, 10899, 11015, 11130, 11245, 11358, 11475, 11590, 11711, 11829,
11956, 12087, 12208, 12333, 12460, 12593, 12726, 12857, 13002, 13143,
13284, 13417, 13558, 13695, 13833, 13974, 14115, 14272, 14415, 14560,
14713, 14862, 15011, 15170, 15325, 15496, 15651, 15808, 15977, 16161,
16336, 16505, 16674, 16851, 17024, 17195, 17376, 17559, 17742, 17929,
18116, 18309, 18503, 18694, 18909, 19126, 19325, 19539, 19740, 19939,
20152, 20355, 20564, 20778, 20988, 21199, 21412, 21629, 21852, 22073,
22301, 22536, 22779, 23010, 23252, 23491, 23730, 23971, 24215, 24476,
24721, 24976, 25230, 25493, 25756, 26022, 26291, 26566, 26838, 27111,
27392, 27682, 27959, 28248, 28548, 28845, 29138, 29434, 29731, 30037,
30346, 30654, 30974, 31285, 31605, 31948, 32272, 32601, 32932, 33282,
33623, 33961, 34302, 34654, 35031, 35395, 35750, 36112, 36479, 36849,
37227, 37606, 37992, 38385, 38787, 39176, 39576, 39980, 40398, 40816,
41226, 41641, 42067, 42490, 42916, 43388, 43840, 44279, 44729, 45183,
45638, 46104, 46574, 47047, 47523, 48007, 48489, 48976, 49470, 49978,
50511, 51017, 51530, 52062, 52586, 53114, 53650, 54188, 54735, 55289,
55843, 56403,
};

const int n_Kprime = (int)(sizeof(Kprime)/sizeof(Kprime[0]));

bool test_syst_inv(int K_lower, int K_upper)
{
	printf("Checking for invertibility from the K source symbols.\n");
	int ntest = 0;
	int i;
	for (i = 0; i < n_Kprime && Kprime[i] <= K_upper; ++i) {
		const int K = Kprime[i];
		if (K < K_lower)
			continue;
		printf("--> Checking K=%d\n", K);
		size_t interWorkSize, interProgSize, interSymNum;
#define RUN_NOFAIL(x) \
	do { \
		int RUN_NOFAIL_err; \
		if ((RUN_NOFAIL_err = (x)) < 0) { \
			fprintf(stderr, "Error:%s:%d: %s failed: %d.\n", \
				__FILE__, __LINE__, #x, RUN_NOFAIL_err); \
			return false; \
		} \
	} while(0)

		RUN_NOFAIL(RqInterGetMemSizes(K, 0,
		  &interWorkSize, &interProgSize, &interSymNum));
		RqInterWorkMem* interWork = malloc(interWorkSize);
		RUN_NOFAIL(RqInterInit(K, 0, interWork, interWorkSize));
		RUN_NOFAIL(RqInterAddIds(interWork, 0, K));
		RqInterProgram* interProg = malloc(interProgSize);
		RUN_NOFAIL(RqInterCompile(interWork, interProg, interProgSize));
#undef RUN_NOFAIL

		free(interProg);
		free(interWork);

		++ntest;
	}
	printf("--> Passed, %d values were tested.\n", ntest);
	return true;
}

static void usage()
{
	puts(	"RQ invertibility test.\n"
	        "Check that the K source symbols uniquely define the "
		"intermediate block\n"
		"\n"
		"   -h          display this help screen and exit\n"
		"   -u #        upper limit of the K value to check\n"
		"\n"
		"Recommended values for tests somewhat more exhaustive than\n"
		"the default:  -u 1000\n"
	);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv)
{
	int K_lower = 1;
	int K_upper = 300;

	/* scan command lines */
	int c;
	while ((c = getopt(argc, argv, "hl:u:i:z:s:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'l':
			K_lower = atoi(optarg);
			break;
		case 'u':
			K_upper = atoi(optarg);
			break;
		case '?':
			exit(EXIT_FAILURE);
		};
	}

	/* Run tests */
	if (!test_syst_inv(K_lower, K_upper))
		return 1;

	return EXIT_SUCCESS;
}
