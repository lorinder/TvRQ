# C
foreach(_target
  rq_failprob
  rq_encdec_match
  rq_syst_inv
  gen_syms)
	add_executable(${_target} ${_target}.c)
	target_link_libraries(${_target} tvrqapi tvrq_test_utils m)
endforeach()

# CXX
foreach(_target
    file_enc_dec
    codec_speed)
  add_executable(${_target} ${_target}.cpp)
  target_link_libraries(${_target} tvrqapi tvrq_test_utils m)
endforeach()

# Tests
foreach(_target
  rq_encdec_match
  rq_syst_inv
)
	add_test(
		NAME		${_target}
		COMMAND		$<TARGET_FILE:${_target}>
	)
endforeach()
