cmake_minimum_required(VERSION 3.10)

project(tvrqapi
  LANGUAGES C CXX
  VERSION 1.0
  DESCRIPTION "TvRQ implementation of a RaptorQ codec"
  )

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# Set the default build type to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Type of build" FORCE)
endif()

# Installation directories
include(GNUInstallDirs)
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/tvrqapi
	CACHE STRING "Relative installation directory for header files")
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR}
	CACHE STRING "Relative installation directory for binaries")
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR}
	CACHE STRING "Relative installation directory for libraries")


enable_testing()

# Add warning flags
if (CMAKE_C_COMPILER_ID MATCHES GNU OR CMAKE_C_COMPILER_ID MATCHES CLANG)
	list(APPEND CMAKE_C_FLAGS -Wall)
	list(APPEND CMAKE_CXX_FLAGS -Wall)
endif()

add_subdirectory(api)
add_subdirectory(algebra)
add_subdirectory(rfc6330_alg)
add_subdirectory(samples_tests)
add_subdirectory(tools)
add_subdirectory(tvrq)
add_subdirectory(in)
