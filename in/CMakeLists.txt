#
# Create and install the cmake package config files
#
include(CMakePackageConfigHelpers)
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	INSTALL_DESTINATION ${INSTALL_LIB_DIR}/cmake/${PROJECT_NAME}
	PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LIB_DIR)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake
	DESTINATION ${INSTALL_LIB_DIR}/cmake/${PROJECT_NAME})

#
# Create and install the pkg-config files
#
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
	DESTINATION ${INSTALL_LIB_DIR}/pkgconfig)
