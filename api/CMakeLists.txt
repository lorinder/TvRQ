add_library(tvrqapi SHARED
	rq_api.h		tvrq_api.c
)
target_include_directories(tvrqapi PUBLIC .)
target_link_libraries(tvrqapi PRIVATE algebra rfc6330_alg tvrq)
target_compile_definitions(tvrqapi PRIVATE RQAPI_BUILD)

set_target_properties(tvrqapi tvrqapi PROPERTIES
	SOVERSION	1
	VERSION		"1.0.0"
	PUBLIC_HEADER	${CMAKE_CURRENT_SOURCE_DIR}/rq_api.h
)

# Installation target
install(TARGETS tvrqapi
	LIBRARY DESTINATION ${INSTALL_LIB_DIR}
	PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDE_DIR}
)
