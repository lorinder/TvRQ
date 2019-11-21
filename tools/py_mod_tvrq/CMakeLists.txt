# Python _tvrq module
if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	# The build rules for the module are currently dependent on
	# linux, so we never build the python module on other systems.
	option(BUILD_PYTHON_MODULE "Build the Python _tvrq module" OFF)
endif()
if(BUILD_PYTHON_MODULE)
	find_package(Python REQUIRED COMPONENTS Interpreter Development)
	execute_process(
	  COMMAND ${Python_EXECUTABLE} -c "import cffi"
	  RESULT_VARIABLE cffi_module_error
	  OUTPUT_QUIET
	  ERROR_QUIET)
  	if(cffi_module_error)
		message(FATAL_ERROR "The required python cffi module is "
		  "not installed or not working.")
	endif()
	set(gen_tvrq_py ${CMAKE_CURRENT_SOURCE_DIR}/gen_tvrq.py)
	add_custom_command(
	  OUTPUT _tvrq.so
	  COMMAND rm -f "_tvrq*.so"
	  COMMAND ${Python_EXECUTABLE} ${gen_tvrq_py}
	  COMMAND test "_tvrq*.so" = "_tvrq.so" || mv -f "_tvrq*.so" _tvrq.so
	  DEPENDS $<TARGET_FILE:tvrq> ${gen_tvrq_py}
	)
	add_custom_target(tvrq_lib ALL DEPENDS _tvrq.so)
endif()
