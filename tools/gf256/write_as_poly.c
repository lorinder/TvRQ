/* tcc -I../../algebra write_as_poly.c gf256.o -run
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void write_as_poly(unsigned int v)
{
	int np = 0;
	for (int i = 15; i >= 0; --i) {
		if (v & (1u << i)) {
			if (np) {
				printf(" + ");
			}
			if (i > 0) {
				printf("x^%d", i);
			} else {
				printf("1");
			}
			++np;
		}
	}

	if (np == 0)
		putc('0', stdout);
	putc('\n', stdout);

}

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i) {
		unsigned int u;
		if (sscanf(argv[i], "%x", &u) != 1) {
			fprintf(stderr, "Error: can't parse `%s' as hex\n",
				argv[i]);
			exit(EXIT_FAILURE);
		}
		write_as_poly(u);
	}
	return EXIT_SUCCESS;
}
