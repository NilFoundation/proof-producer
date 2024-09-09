#---------------------------------------------------------------------------#
# Copyright (c) 2022 Mikhail Komarov <nemo@nil.foundation>
# Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
# Copyright (c) 2024 Iosif (x-mass) <x-mass@nil.foundation>
#
# Distributed under the Boost Software License, Version 1.0
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt
#---------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/packages"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/share/modules/cmake"
)

cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0057 NEW)

include(CMConfig)
include(CMDeploy)
include(CMSetupVersion)

cm_project(proof-producer WORKSPACE_NAME ${CMAKE_WORKSPACE_NAME} LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(crypto3 REQUIRED)

find_package(actor_zk REQUIRED)
find_package(actor_containers REQUIRED)
find_package(actor_core REQUIRED)
find_package(actor_math REQUIRED)

# TODO: move thread to actor-core
find_package(Boost REQUIRED COMPONENTS filesystem log program_options thread)
if(ENABLE_TESTS)
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)
endif()

# EXCLUDE_FROM_ALL is needed so crypto3 and other headers won't be installed
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/transpiler" EXCLUDE_FROM_ALL)

# Read version from VERSION file
set(VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/VERSION")
file(READ ${VERSION_FILE} PROOF_PRODUCER_VERSION)
string(STRIP ${PROOF_PRODUCER_VERSION} PROOF_PRODUCER_VERSION)
# Remove leading 'v' from tag
string(REGEX REPLACE "^v" "" PROOF_PRODUCER_VERSION ${PROOF_PRODUCER_VERSION})
if(NOT PROOF_PRODUCER_VERSION)
    message(FATAL_ERROR
            "Unable to retrive version from git or ${VERSION_FILE} file.")
endif()

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

set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/bin/proof-producer")

include(CPack)
