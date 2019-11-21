foreach(_target
  gf256
  m256v_basic
  m256v_lu
  m256v_splitsolve
  m2v_basic
  m2v_lu
  mv_submat
)
	add_executable(${_target} ${_target}.c)
	target_link_libraries(${_target} tvrq tvrq_test_utils m)
endforeach()

# Tests
foreach(_target
  gf256
  m256v_basic
  m256v_lu
  m256v_splitsolve
  m2v_basic
  m2v_lu
)
	add_test(
		NAME		${_target}
		COMMAND		$<TARGET_FILE:${_target}>
	)
endforeach()
