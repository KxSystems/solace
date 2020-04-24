cmake_minimum_required(VERSION 2.8)
project(solacekdb CXX)

set (CMAKE_BUILD_TYPE Release)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-strict-aliasing -DKXVER=3")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-strict-aliasing -DKXVER=3")

set(MY_LIBRARY_NAME kdbsolace)
file(GLOB CPP_FILES source/*.cpp)
set_source_files_properties(${CPP_FILES} PROPERTIES LANGUAGE CXX)
message(STATUS "Using $ENV{SOLACE_API_DIR} for Solace API")

include_directories (
    $ENV{SOLACE_API_DIR}/include
)

link_directories(
    $ENV{SOLACE_API_DIR}/lib/
)

add_library (${MY_LIBRARY_NAME} SHARED ${CPP_FILES})

target_link_libraries(${MY_LIBRARY_NAME} solclient)

set_target_properties(${MY_LIBRARY_NAME} PROPERTIES
    PUBLIC_HEADER source/kdbsolace.h)


install(TARGETS ${MY_LIBRARY_NAME}
    LIBRARY DESTINATION ${PROJECT_BINARY_DIR}/api/lib
    PUBLIC_HEADER DESTINATION ${PROJECT_BINARY_DIR}/api/include)

install(FILES README.md DESTINATION ${PROJECT_BINARY_DIR}/api/)

install(DIRECTORY examples DESTINATION ${PROJECT_BINARY_DIR}/api)
install(DIRECTORY script DESTINATION ${PROJECT_BINARY_DIR}/api)
