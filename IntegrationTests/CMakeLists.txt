#############################################
# CMake build script for IntegrationTests
#

cmake_minimum_required(VERSION 3.0)

project(IntegrationTests)

find_package(boost    REQUIRED)
find_package(poco     REQUIRED)
find_package(rapidxml REQUIRED)
find_package(sqlite3  REQUIRED)

#####################
# Macro definitions:

add_definitions(
    -DENABLE_3FD_CST
    -DENABLE_3FD_ERR_IMPL_DETAILS
    -DTESTING
)

########################
# Include directories:

include_directories(
    "${PROJECT_SOURCE_DIR}/../3FD"
    "${PROJECT_SOURCE_DIR}/../gtest/include"
    "${PROJECT_SOURCE_DIR}/../OpenCL"
    "${PROJECT_SOURCE_DIR}/../btree"
)

########################
# Dependency libraries:

# How and what libs to link:
add_library(3FD   STATIC IMPORTED)
add_library(gtest STATIC IMPORTED)

# Where the lib binaries are:
string(TOLOWER ${CMAKE_BUILD_TYPE} buildType)
if(buildType STREQUAL release)
    add_definitions(-DNDEBUG)
    set_target_properties(3FD   PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/../3FD/lib3FD.a")
    set_target_properties(gtest PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/../gtest/libgtest.a")
elseif(buildType STREQUAL debug)
    set_target_properties(3FD   PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/../3FD/lib3FDd.a")
    set_target_properties(gtest PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/../gtest/libgtestd.a")
endif()

# Place the configuration file along with the executable:
add_custom_command(
   OUTPUT IntegrationTests.3fd.config
   COMMAND cp ${PROJECT_SOURCE_DIR}/application.config ${PROJECT_SOURCE_DIR}/../build/bin/IntegrationTests.3fd.config
   DEPENDS ${PROJECT_SOURCE_DIR}/application.config
)

# Executable source files:
add_executable(IntegrationTests
    IntegrationTests.cpp
    tests_broker.cpp
    tests_core.cpp
    tests_gc.cpp
    tests_opencl.cpp
    tests_sqlite.cpp
    IntegrationTests.3fd.config
)

# Linking:
target_link_libraries(IntegrationTests 
    gtest
    3FD
    odbc pthread dl
    boost_system boost_thread
    PocoDataODBC PocoData PocoUtil PocoXML PocoFoundation
	rapidxml
	sqlite3
)

################
# Installation:

install(
    TARGETS IntegrationTests
    DESTINATION "${PROJECT_SOURCE_DIR}/../build/bin"
)
