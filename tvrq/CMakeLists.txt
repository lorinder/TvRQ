add_library(tvrq STATIC
	hdpc.h		hdpc.c
	ldpc.h		ldpc.c
	lt.h		lt.c
	rq_matrix.h	rq_matrix.c
)
target_include_directories(tvrq PUBLIC .)
target_link_libraries(tvrq PUBLIC algebra rfc6330_alg)
