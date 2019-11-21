add_library(algebra STATIC
	gf256.h			gf256.c
	m2v.h			m2v.c
	m256v.h			m256v.c
	mv_generic.h
)
target_include_directories(algebra PUBLIC .)
