##%% General Settings %%##vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv#

# CMake Version
cmake_minimum_required(VERSION 3.1)

# Name of package
project(kdbsolace CXX)

# Name of shared object
set(MY_LIBRARY_NAME solacekdb)

# Dispay settings
message(STATUS "Generator : ${CMAKE_GENERATOR}")
message(STATUS "Generator Platform : ${CMAKE_GENERATOR_PLATFORM}")
message(STATUS "Build Tool : ${CMAKE_BUILD_TOOL}")
message(STATUS "Solace API : $ENV{SOLACE_INSTALL_DIR}")

##%% Compilation %%##vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv#

# Set default build type Release
if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif()

# Specify source files
file(GLOB SRC_FILES src/*.cpp)
set_source_files_properties(${SRC_FILES} PROPERTIES LANGUAGE CXX)

# Set target shared object
add_library(${MY_LIBRARY_NAME} SHARED ${SRC_FILES})

# Specify include directories
target_include_directories (${MY_LIBRARY_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    $ENV{SOLACE_INSTALL_DIR}/include
)

# Basic compile flag
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DKXVER=3")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -DKXVER=3")

# Download k.h
file(DOWNLOAD "https://github.com/KxSystems/kdb/raw/master/c/c/k.h" "${PROJECT_SOURCE_DIR}/include/k.h")

if(MSVC)
    # Windows Visual Studio Win32 or Win64
    # Search Solace C API dependency
    find_library(SOLCLIENT_LIBRARY
        NAMES solclient libsolclient
        HINTS "$ENV{SOLACE_INSTALL_DIR}/lib/${CMAKE_GENERATOR_PLATFORM}"
    )
    # Get q dependency
    file(DOWNLOAD "https://github.com/KxSystems/kdb/raw/master/w32/q.lib" "${CMAKE_BINARY_DIR}/q.lib" )
    set(LINK_LIBS "${CMAKE_BINARY_DIR}/q.lib")
else()
    # Linux or MacOSX
    # Search Solace C API dependency
    find_library(SOLCLIENT_LIBRARY
        NAMES solclient libsolclient
        HINTS "$ENV{SOLACE_INSTALL_DIR}/lib/"
    )
    # Additional compile flags
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-strict-aliasing")
endif()

# Shared library prefix and suffix
# ex.) solacekdb.so for linux
# Suffix must be `.so` for Mac
if(APPLE)
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
endif()
set_target_properties(${MY_LIBRARY_NAME} PROPERTIES PREFIX "")
set_target_properties(${MY_LIBRARY_NAME} PROPERTIES SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})

# Link flag
IF(APPLE)
   set_target_properties(${MY_LIBRARY_NAME} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

# Link dependent libraries
target_link_libraries(${MY_LIBRARY_NAME} ${SOLCLIENT_LIBRARY} ${LINK_LIBS})

##%% Installation %%##vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv#

# Set install script name to be packaged
if(MSVC)
    if(CMAKE_GENERATOR_PLATFORM MATCHES "Win32")
        set(INSTALL_SCRIPT "install32.bat")
    else()
        # Win64
        set(INSTALL_SCRIPT "install.bat")
    endif()
    set(OSFLAG w)
else()
    # Linux or MacOSX
    set(INSTALL_SCRIPT "install.sh")
    if(APPLE)
        # MacOSX
        set(OSFLAG m)
    else()
        # Linux
        set(OSFLAG l)
    endif()
endif()

# Set bitness
set(BITNESS 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITNESS 64)
endif()

# Always build package
file(COPY README.md LICENSE ${INSTALL_SCRIPT} DESTINATION ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}/)
file(COPY examples/ DESTINATION ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}/examples/)
file(COPY q/ DESTINATION ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}/q/)

# Copy built shared object into package directory after build instead of during installation
add_custom_command(TARGET ${MY_LIBRARY_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${MY_LIBRARY_NAME}>" ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}/lib/${MY_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
    DEPENDS ${MY_LIBRARY_NAME}
)

# Install
install(TARGETS ${MY_LIBRARY_NAME} DESTINATION $ENV{QHOME}/${OSFLAG}${BITNESS} CONFIGURATIONS Release)
