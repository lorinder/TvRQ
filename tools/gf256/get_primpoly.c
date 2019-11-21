/* tcc -I../../algebra get_primpoly.c gf256.o -run
 */
#include <stdio.h>

#include "gf256.h"

int main(void)
{
	printf("primitive poly w/o leading term: %hhx\n",
		gf256_mul(0x80, 0x2));
	return 0;
}
