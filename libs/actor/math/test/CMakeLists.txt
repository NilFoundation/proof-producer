#---------------------------------------------------------------------------#
# Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
# Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
#
# Distributed under the Boost Software License, Version 1.0
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt
#---------------------------------------------------------------------------#

include(CMTest)

if(NOT Boost_UNIT_TEST_FRAMEWORK_FOUND)
    cm_find_package(Boost REQUIRED COMPONENTS unit_test_framework)
endif()

function(actor_add_test name)
    get_filename_component(test_name ${name} NAME)
    string(REPLACE "/" "_" target_name ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}_${test_name}_test)

    set(test_kinds
            ACTOR
            BOOST
            CUSTOM)

    cmake_parse_arguments(parsed_args
            ""
            "WORKING_DIRECTORY;KIND"
            "RUN_ARGS;SOURCES;LIBRARIES;DEPENDS"
            ${ARGN})

    if(NOT parsed_args_KIND)
        set(parsed_args_KIND ACTOR)
    elseif(NOT (parsed_args_KIND IN_LIST test_kinds))
        message(FATAL_ERROR "Invalid test kind. KIND must be one of ${test_kinds}")
    endif()

    if(parsed_args_SOURCES)
        #
        # Each kind of test must populate the `args` and `libraries` lists.
        #

        set(libraries "${parsed_args_LIBRARIES}")
        list(APPEND libraries
                ${CMAKE_WORKSPACE_NAME}::core
                ${CMAKE_WORKSPACE_NAME}::network
                ${CMAKE_WORKSPACE_NAME}::testing
                #               epoll-shim

                crypto3::algebra
                crypto3::math
                crypto3::multiprecision)


        set(args "")
        if(parsed_args_KIND STREQUAL "ACTOR")
            list(APPEND args -- -c ${BUILD_WITH_UNIT_TEST_SMP})
        endif()

        if(NOT (UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux")))
            list(APPEND libraries epoll-shim)
        endif()

        list(APPEND args ${parsed_args_RUN_ARGS})

        cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                ${libraries}
                ${Boost_LIBRARIES})

        cm_test(NAME ${target_name} SOURCES ${parsed_args_SOURCES})

        target_compile_definitions(${target_name} PRIVATE
                ACTOR_TESTING_MAIN BOOST_TEST_DYN_LINK)

        if((BUILD_WITH_STACK_GUARDS STREQUAL "ON") OR
        ((BUILD_WITH_STACK_GUARDS STREQUAL "DEFAULT") AND
        (CMAKE_BUILD_TYPE IN_LIST ACTOR_STACK_GUARD_MODES)))
            target_compile_definitions(${target_name}
                    PRIVATE ACTOR_THREAD_STACK_GUARDS)
        endif()

        target_include_directories(${target_name} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${BUILD_WITH_SOURCE_DIR}/src)

        set_target_properties(${target_name} PROPERTIES
                CXX_STANDARD 17
                CXX_STANDARD_REQUIRED TRUE)

        target_compile_options(${target_name} PRIVATE
                ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS})

        get_target_property(target_type Boost::unit_test_framework TYPE)
        if (target_type STREQUAL "SHARED_LIB")
            target_compile_definitions(${target_name} PRIVATE BOOST_TEST_DYN_LINK)
        elseif (target_type STREQUAL "STATIC_LIB")

        endif ()
    else()
        if(NOT (parsed_args_KIND STREQUAL "CUSTOM"))
            message(FATAL_ERROR "SOURCES are required for ${parsed_args_KIND} tests")
        endif()
    endif()


endfunction()

set(TESTS_NAMES
    "evaluation_domain"
    "expression"
    "kronecker_substitution"
    "polynomial_arithmetic"
    "polynomial"
    "polynomial_view"
    "polynomial_dfs"
    "polynomial_dfs_view"
    "lagrange_interpolation")

foreach (TEST_NAME ${TESTS_NAMES})
    actor_add_test(${TEST_NAME} SOURCES ${TEST_NAME}.cpp)
endforeach ()
