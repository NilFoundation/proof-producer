#---------------------------------------------------------------------------//
# Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#---------------------------------------------------------------------------//

include(CMTest)

set(BUILD_WITH_UNIT_TEST_SMP
    2
    CACHE
    STRING
    "Run unit tests with this many cores.")

#
# Define a new unit test with the given name.
#
# actor_add_test(name
#   [KIND {ACTOR,BOOST,CUSTOM}]
#   [SOURCES source1 source2 ... sourcen]
#   [WORKING_DIRECTORY dir]
#   [LIBRARIES library1 library2 ... libraryn]
#   [RUN_ARGS arg1 arg2 ... argn])
#
# There are three kinds of test we support (the KIND parameter):
#
# - ACTOR: Unit tests which use macros like `ACTOR_TEST_CASE`
# - BOOST: Unit tests which use macros like `BOOST_AUTO_TEST_CASE`
# - CUSTOM: Custom tests which need to be specified
#
# ACTOR and BOOST tests will have their output saved for interpretation by the Jenkins continuous integration service
# if this is configured for the build.
#
# KIND can be omitted, in which case it is assumed to be ACTOR.
#
# If SOURCES is provided, then the test files are first compiled into an executable which has the same name as the test
# but with a suffix ("_test").
#
# WORKING_DIRECTORY can be optionally provided to choose where the test is executed.
#
# If LIBRARIES is provided along with SOURCES, then the executable is additionally linked with these libraries.
#
# RUN_ARGS are optional additional arguments to pass to the executable. For ACTOR tests, these come after `--`. For
# CUSTOM tests with no SOURCES, this parameter can be used to specify the executable name as well as its arguments since
# no executable is compiled.
#
function(actor_add_test name)
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

        set(args "")
        if(parsed_args_KIND STREQUAL "ACTOR")
            list(APPEND libraries
                 ${CMAKE_WORKSPACE_NAME}::core
                 ${CMAKE_WORKSPACE_NAME}::network
                 ${CMAKE_WORKSPACE_NAME}::testing)

            list(APPEND args -- -c ${BUILD_WITH_UNIT_TEST_SMP})
        elseif(parsed_args_KIND STREQUAL "BOOST")
            list(APPEND libraries
                 ${CMAKE_WORKSPACE_NAME}::core
                 ${CMAKE_WORKSPACE_NAME}::network
                 ${CMAKE_WORKSPACE_NAME}::testing)
        endif()

        if(NOT (UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux")))
            list(APPEND libraries epoll-shim)
        endif()

        list(APPEND args ${parsed_args_RUN_ARGS})

        cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
                               ${libraries}
                               ${Boost_LIBRARIES})

        cm_test(NAME ${CMAKE_WORKSPACE_NAME}_${name}_test SOURCES ${parsed_args_SOURCES})

        target_compile_definitions(${CMAKE_WORKSPACE_NAME}_${name}_test PRIVATE
                                   ACTOR_TESTING_MAIN BOOST_TEST_DYN_LINK)

        if((BUILD_WITH_STACK_GUARDS STREQUAL "ON") OR
        ((BUILD_WITH_STACK_GUARDS STREQUAL "DEFAULT") AND
        (CMAKE_BUILD_TYPE IN_LIST ACTOR_STACK_GUARD_MODES)))
            target_compile_definitions(${CMAKE_WORKSPACE_NAME}_${name}_test
                                       PRIVATE ACTOR_THREAD_STACK_GUARDS)
        endif()

        target_include_directories(${CMAKE_WORKSPACE_NAME}_${name}_test PRIVATE
                                   ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${BUILD_WITH_SOURCE_DIR}/src)

        set_target_properties(${CMAKE_WORKSPACE_NAME}_${name}_test PROPERTIES
                              CXX_STANDARD 17
                              CXX_STANDARD_REQUIRED TRUE)

        target_compile_options(${CMAKE_WORKSPACE_NAME}_${name}_test PRIVATE
                               ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS})
    else()
        if(NOT (parsed_args_KIND STREQUAL "CUSTOM"))
            message(FATAL_ERROR "SOURCES are required for ${parsed_args_KIND} tests")
        endif()
    endif()
endfunction()

#
# Define a new custom unit test whose entry point is a Actor application.
#
# actor_add_app_test(name
#   [SOURCES source1 source2 ... sourcen]
#   [LIBRARIES library1 library2 ... libraryn]
#   [RUN_ARGS arg1 arg2 ... argn])
#
# These kinds of tests are structured like Actor applications.
#
# These tests always link against `actor_private` and are always invoked with
# `-c ${BUILD_WITH_UNIT_TEST_SMP}`.
#
function(actor_add_app_test name)
    cmake_parse_arguments(parsed_args
                          ""
                          ""
                          "RUN_ARGS;SOURCES;LIBRARIES"
                          ${ARGN})

    actor_add_test(${name}
                   KIND CUSTOM
                   SOURCES ${parsed_args_SOURCES}
                   LIBRARIES
                   ${CMAKE_WORKSPACE_NAME}::core
                   ${CMAKE_WORKSPACE_NAME}::network
                   ${CMAKE_WORKSPACE_NAME}::testing
                   ${parsed_args_LIBRARIES}
                   RUN_ARGS
                   -c ${BUILD_WITH_UNIT_TEST_SMP}
                   ${parsed_args_RUN_ARGS})
endfunction()

function(prepend_each var prefix)
    set(result "")

    foreach(x ${ARGN})
        list(APPEND result ${prefix}/${x})
    endforeach()

    set(${var} ${result} PARENT_SCOPE)
endfunction()

actor_add_test(abort_source SOURCES abort_source_test.cc)

actor_add_test(alloc SOURCES alloc_test.cc)

if(NOT ACTOR_EXECUTE_ONLY_FAST_TESTS)
    set(allocator_test_args "")
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(allocator_test_args --iterations 5)
    else()
        set(allocator_test_args --time 0.1)
    endif()
endif()

actor_add_test(allocator
               SOURCES allocator_test.cc
               RUN_ARGS ${allocator_test_args})

if(UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    actor_add_app_test(alien
                       SOURCES alien_test.cc)
endif()

actor_add_test(checked_ptr
               SOURCES checked_ptr_test.cc)

actor_add_test(chunked_fifo
               SOURCES chunked_fifo_test.cc)

actor_add_test(circular_buffer
               SOURCES circular_buffer_test.cc)

actor_add_test(circular_buffer_fixed_capacity
               SOURCES circular_buffer_fixed_capacity_test.cc)

actor_add_test(coroutines
               SOURCES coroutines_test.cc)

actor_add_test(defer
               SOURCES defer_test.cc)

actor_add_test(deleter
               SOURCES deleter_test.cc)

actor_add_app_test(directory
                   SOURCES directory_test.cc)

actor_add_test(distributed
               SOURCES distributed_test.cc)

actor_add_test(execution_stage
               SOURCES execution_stage_test.cc)

actor_add_test(expiring_fifo
               SOURCES expiring_fifo_test.cc)

actor_add_test(io_queue
               SOURCES io_queue_test.cc)

actor_add_test(fair_queue
               SOURCES fair_queue_test.cc)

actor_add_test(file_io
               SOURCES file_io_test.cc)

actor_add_test(file_utils
               SOURCES file_utils_test.cc)

actor_add_test(foreign_ptr
               SOURCES foreign_ptr_test.cc)

actor_add_test(fstream
               SOURCES
               fstream_test.cc
               mock_file.hh)

actor_add_test(futures
               SOURCES futures_test.cc)

actor_add_test(sharded
               SOURCES sharded_test.cc)

actor_add_test(locking
               SOURCES locking_test.cc)

actor_add_test(lowres_clock
               SOURCES lowres_clock_test.cc)

actor_add_test(metrics
               SOURCES metrics_test.cc)

actor_add_test(noncopyable_function
               KIND BOOST
               SOURCES noncopyable_function_test.cc)

actor_add_test(program_options
               KIND BOOST
               SOURCES program_options_test.cc)

actor_add_test(queue
               SOURCES queue_test.cc)

actor_add_test(semaphore
               SOURCES semaphore_test.cc)

actor_add_test(shared_ptr
               KIND BOOST
               SOURCES shared_ptr_test.cc)

actor_add_test(signal
               SOURCES signal_test.cc)

actor_add_test(simple_stream
               KIND BOOST
               SOURCES simple_stream_test.cc)

# TODO: Disabled for now. See GH-520.
# actor_add_test(slab
#   SOURCES slab_test.cc
#   NO_ACTOR_TESTING_LIBRARY)

actor_add_app_test(smp
                   SOURCES smp_test.cc)

actor_add_test(sstring
               KIND BOOST
               SOURCES sstring_test.cc)

actor_add_test(stall_detector
               SOURCES stall_detector_test.cc)

actor_add_test(thread
               SOURCES thread_test.cc)

actor_add_test(scheduling_group
               SOURCES scheduling_group_test.cc)

actor_add_app_test(thread_context_switch
                   SOURCES thread_context_switch_test.cc)

actor_add_app_test(timer
                   SOURCES timer_test.cc)

actor_add_test(uname
               KIND BOOST
               SOURCES uname_test.cc)

actor_add_test(tuple_utils
               KIND BOOST
               SOURCES tuple_utils_test.cc)

actor_add_test(unwind
               KIND BOOST
               SOURCES unwind_test.cc)

actor_add_test(weak_ptr
               KIND BOOST
               SOURCES weak_ptr_test.cc)

actor_add_test(log_buf
               SOURCES log_buf_test.cc)
