foreach(_target
  parameters
  ldpc
  hdpc
  lt
  rq_matrix)
	add_executable(${_target} ${_target}.c)
	target_link_libraries(${_target} tvrq m)
endforeach()

target_link_libraries(rq_matrix tvrq_test_utils)
target_link_libraries(lt tvrq_test_utils)
