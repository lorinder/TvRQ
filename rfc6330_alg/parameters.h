#ifndef PARAMETERS_H
#define PARAMETERS_H

// Sect 5.1.2
typedef struct {
	int K;		// Number of symbols in a single source block
	int Kprime;	// Number of source symbols with padding
	int J;		// Systematic index
	int L;		// Number of intermediate symbols
	int S;		// Number of LDPC symbols
	int H;		// Number of HDPC symbols
	int B;		// Number of intermediate LT symbols w/o LDPC
	int W;		// Number of intermediate LT symbols w/ LDPC
	int P;		// Number of PI symbols
	int P1;		// Smallest prime >= P
	int U;		// Number of non-HDPC intermediate PI symbols
} parameters;

/**	Compute all code parameters given K.
 *
 *	@return		A parameter struct for the given K.  If K is
 *			invalid, the returned struct will have a K
 *			member of -1.
 */
parameters parameters_get(int K);


/**	Facility to dump all parameters.
 */
typedef void (*parameter_print_func)(void* usr,
				const char* param_name,
				int value);

void parameters_dump(const parameters* P,
			void* usr,
			parameter_print_func f);

#endif /* PARAMETERS_H */
