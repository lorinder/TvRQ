add_library(tvrq_test_utils STATIC
  m2v_m256v_mat_pair.h		m2v_m256v_mat_pair.c
  parse_esis.h			parse_esis.c
  perm.h			perm.c
  test_utils.h			test_utils.c
)
target_link_libraries(tvrq_test_utils PUBLIC algebra)
target_include_directories(tvrq_test_utils PUBLIC .)
