#---------------------------------------------------------------------------#
# Copyright (c) 2022 Mikhail Komarov <nemo@nil.foundation>
# Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
# Copyright (c) 2023 Ilia Shirobokov <i.shirobokov@nil.foundation>
# Copyright (c) 2024 Iosif (x-mass) <x-mass@nil.foundation>
#
# Distributed under the Boost Software License, Version 1.0
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt
#---------------------------------------------------------------------------#

include(CMDeploy)
include(CMSetupVersion)

if (CPACK_PACKAGE_VERSION)
    add_compile_definitions(PROOF_GENERATOR_VERSION=${CPACK_PACKAGE_VERSION})
endif()

option(ZK_PLACEHOLDER_PROFILING_ENABLED "Build with placeholder profiling" FALSE)
option(ZK_PLACEHOLDER_DEBUG_ENABLED "Build with placeholder testing inside" FALSE)

if(ZK_PLACEHOLDER_PROFILING)
    add_definitions(-DZK_PLACEHOLDER_PROFILING_ENABLED)
endif()

if(ZK_PLACEHOLDER_DEBUG)
    add_definitions(-DZK_PLACEHOLDER_DEBUG_ENABLED)
endif()

# Function to setup common properties for a target
function(setup_proof_generator_target)
    set(options "")
    set(oneValueArgs TARGET_NAME)
    set(multiValueArgs ADDITIONAL_DEPENDENCIES)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${ARG_TARGET_NAME}
        src/arg_parser.cpp
        src/main.cpp
    )

    # Make sure to add these dependencies before the others, since for multi-threaded version dependency on
    # actor-zk must come first.
    if(ARG_ADDITIONAL_DEPENDENCIES)
        foreach(lib IN LISTS ARG_ADDITIONAL_DEPENDENCIES)
            target_link_libraries(${ARG_TARGET_NAME} ${lib})
        endforeach()
    endif()

    set_target_properties(${ARG_TARGET_NAME} PROPERTIES
        LINKER_LANGUAGE CXX
        EXPORT_NAME ${ARG_TARGET_NAME}
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED TRUE)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${ARG_TARGET_NAME} PRIVATE "-fconstexpr-steps=2147483647")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${ARG_TARGET_NAME} PRIVATE "-fconstexpr-ops-limit=4294967295")
    endif ()

    target_include_directories(${ARG_TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<$<BOOL:${Boost_FOUND}>:${Boost_INCLUDE_DIRS}>)

    target_link_libraries(${ARG_TARGET_NAME}
        crypto3::algebra
        crypto3::block
        crypto3::codec
        crypto3::multiprecision
        crypto3::pkpad
        crypto3::pubkey
        crypto3::random

        marshalling::core
        marshalling::crypto3_algebra
        marshalling::crypto3_multiprecision
        marshalling::crypto3_zk

        crypto3::transpiler

        Boost::filesystem
        Boost::log
        Boost::program_options
        Boost::thread
    )
endfunction()

# Declare single-threaded target
set(SINGLE_THREADED_TARGET "${CURRENT_PROJECT_NAME}-single-threaded")
setup_proof_generator_target(TARGET_NAME ${SINGLE_THREADED_TARGET} ADDITIONAL_DEPENDENCIES crypto3::zk)

# Declare multi-threaded target
set(MUTLI_THREADED_TARGET "${CURRENT_PROJECT_NAME}-multi-threaded")
setup_proof_generator_target(TARGET_NAME ${MUTLI_THREADED_TARGET} ADDITIONAL_DEPENDENCIES actor::zk)

install(TARGETS ${SINGLE_THREADED_TARGET} ${MUTLI_THREADED_TARGET} RUNTIME DESTINATION bin)
