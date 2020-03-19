cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DKXVER=3")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DKXVER=3")

set (SOLACE_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/)
set (MY_LIBRARY_NAME deltasolace)
set (source_files deltasolace.c)
set_source_files_properties(${source_files} PROPERTIES LANGUAGE CXX)

if(DEFINED ENV{SOLACE_API})
  set (SOLACE_API_DIR $ENV{SOLACE_API})
endif()

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/.. CACHE PATH "Install to cmake source" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

include_directories (
    ${SOLACE_API_DIR}/include
)

link_directories(
    ${SOLACE_API_DIR}/lib/}
)
  
add_library (${MY_LIBRARY_NAME} SHARED ${source_files})
add_library (solclient SHARED IMPORTED)
set_property (TARGET solclient PROPERTY IMPORTED_LOCATION ${SOLACE_API_DIR}/lib/libsolclient.so)

target_link_libraries(${MY_LIBRARY_NAME} solclient)

###########
# Install #
###########
install (TARGETS ${MY_LIBRARY_NAME} DESTINATION clib)

install (DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/api/lib/
        DESTINATION clib
        PATTERN ".svn" EXCLUDE)
