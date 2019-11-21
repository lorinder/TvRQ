add_library(rfc6330_alg STATIC
	parameters.h		parameters.c
	tuple.h			tuple.c
	rand.h			rand.c
)
target_include_directories(rfc6330_alg PUBLIC .)
