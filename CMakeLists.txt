#---------------------------------------------------------------------------#
# Copyright (c) 2022 Mikhail Komarov <nemo@nil.foundation>
# Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
#
# Distributed under the Boost Software License, Version 1.0
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt
#---------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0057 NEW)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake"
        "${CMAKE_CURRENT_LIST_DIR}/cmake/packages"
        "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/share/modules/cmake")

include(CMConfig)
include(CMDeploy)
include(CMSetupVersion)
include_directories(${Boost_INCLUDE_DIRS})

cm_workspace(proof-producer)

option(BUILD_TESTS "Build unit tests" FALSE)
option(BUILD_EXAMPLES "Build unit tests" FALSE)

macro(cm_find_package NAME)
if(NOT "${NAME}" MATCHES "^${CMAKE_WORKSPACE_NAME}_.*$" AND NOT "${NAME}" STREQUAL CM)
find_package(${ARGV})
else()
set(${ARGV0}_FOUND ON CACHE BOOL "")
endif()
endmacro()

option(BUILD_TESTS "Build unit tests" FALSE)
option(BUILD_SHARED_LIBS "Build shared library" TRUE)
option(BUILD_WITH_BOOST_STATIC_LIBS "Build using Boost static libraries" TRUE)

if(BUILD_WITH_BOOST_STATIC_LIBS)
    set(Boost_USE_STATIC_LIBS TRUE)
else()
    set(Boost_USE_STATIC_LIBS FALSE)
endif()

# EXCLUDE_FROM_ALL is needed so crypto3 and other headers won't be installed
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/crypto3" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/module" EXCLUDE_FROM_ALL)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/actor/container" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/actor/core" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/actor/math" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/actor/zk" EXCLUDE_FROM_ALL)

if(NOT PROOF_PRODUCER_VERSION)
    find_program(GIT_EXECUTABLE NAMES git PATHS /usr/bin /usr/local/bin)
    message(STATUS "Running command: ${GIT_EXECUTABLE} describe --tags HEAD")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags HEAD
                    OUTPUT_VARIABLE DESCRIBE_OUTPUT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    message(STATUS "Git describe output: ${DESCRIBE_OUTPUT}")

    set(PROOF_PRODUCER_VERSION "${DESCRIBE_OUTPUT}")

    if(NOT PROOF_PRODUCER_VERSION)
        message(FATAL_ERROR
                "Unable to retrive version from git or ${VERSION_FILE} file.")
    endif()
endif()

# Remove preceding 'v' from tag
string(REGEX REPLACE "^v" "" PROOF_PRODUCER_VERSION ${PROOF_PRODUCER_VERSION})
# Split numbers
string(REPLACE "-" "." PROOF_PRODUCER_VERSION_LIST ${PROOF_PRODUCER_VERSION})
string(REPLACE "." ";" PROOF_PRODUCER_VERSION_LIST ${PROOF_PRODUCER_VERSION_LIST})

list(LENGTH PROOF_PRODUCER_VERSION_LIST VERSION_LIST_LENGHT)

list(GET PROOF_PRODUCER_VERSION_LIST 0 PROOF_PRODUCER_VERSION_MAJOR)
list(GET PROOF_PRODUCER_VERSION_LIST 1 PROOF_PRODUCER_VERSION_MINOR)
list(GET PROOF_PRODUCER_VERSION_LIST 2 PROOF_PRODUCER_VERSION_PATCH)
if(VERSION_LIST_LENGHT GREATER 3)
    list(GET PROOF_PRODUCER_VERSION_LIST 3 PROOF_PRODUCER_VERSION_INCREMENT)
endif()

set(CPACK_GENERATOR DEB)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "=nil; Foundation")

set(CPACK_PACKAGE_VERSION "${PROOF_PRODUCER_VERSION_MAJOR}.${PROOF_PRODUCER_VERSION_MINOR}.${PROOF_PRODUCER_VERSION_PATCH}")
if(PROOF_PRODUCER_VERSION_INCREMENT)
    string(APPEND CPACK_PACKAGE_VERSION "-${PROOF_PRODUCER_VERSION_INCREMENT}")
endif()

if (NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr/lib/proof-producer")
endif()

set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/bin/proof-generator")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/bin/jsonizer")

install(SCRIPT cmake/create-symlinks.cmake)

include(CPack)
