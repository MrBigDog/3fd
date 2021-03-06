####################################
# CMake build script for 3FD
#

cmake_minimum_required(VERSION 2.6)

project(3FD)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-show-template-tree -fno-elide-type")
endif()

#####################
# Macro definitions:

add_definitions(
    -DENABLE_3FD_CST
    -DENABLE_3FD_ERR_IMPL_DETAILS
)

# NDEBUG when release mode:
string(TOLOWER ${CMAKE_BUILD_TYPE} buildType)
if(buildType STREQUAL release)
    add_definitions(-DNDEBUG)
endif()

########################
# Include directories:

include_directories(
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/../btree"
    "${PROJECT_SOURCE_DIR}/../OpenCL"
    "${PROJECT_SOURCE_DIR}/../build/include"
)

########################
# Dependency libraries:

# Static library source files:
add_library(3FD STATIC
    broker_impl.cpp
    broker_impl_reader.cpp
    broker_impl_writer.cpp
    callstacktracer.cpp
    cmdline.cpp
    configuration.cpp
    dependencies.cpp
    exceptions.cpp
    gc_addresseshashtable.cpp
    gc_arrayofedges.cpp
    gc_garbagecollector.cpp
    gc_memorydigraph.cpp
    gc_messages.cpp
    gc_vertex.cpp
    gc_vertexstore.cpp
    logger.cpp
    logger_poco.cpp
    opencl_impl.cpp
    opencl_impl_commandtracker.cpp
    opencl_impl_context.cpp
    opencl_impl_device.cpp
    opencl_impl_platform.cpp
    opencl_impl_program.cpp
    opencl_impl_programmanifest.cpp
    runtime.cpp
    sqlite_databaseconn.cpp
    sqlite_dbconnpool.cpp
    sqlite_prepstatement.cpp
    sqlite_transaction.cpp
    utils_asynchronous.cpp
    utils_dynmempool.cpp
    utils_event.cpp
    utils_io.cpp
    utils_memorypool.cpp
)

################
# Installation:

install(
    TARGETS 3FD
    DESTINATION "${PROJECT_SOURCE_DIR}/../build/lib"
)
