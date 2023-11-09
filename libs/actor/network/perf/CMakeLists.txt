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

# Logical target for all perf tests.
add_custom_target(perf_tests)

macro(actor_add_test name)
    set(args ${ARGN})

    cmake_parse_arguments(
            parsed_args
            "NO_ACTOR_PERF_TESTING_LIBRARY"
            ""
            "SOURCES"
            ${args})

    set(target test_perf_${name})
    add_executable(${target} ${parsed_args_SOURCES})

    if(parsed_args_NO_ACTOR_PERF_TESTING_LIBRARY)
        set(libraries
            ${CMAKE_WORKSPACE_NAME}::core
            ${CMAKE_WORKSPACE_NAME}::network)
    else()
        set(libraries
            ${CMAKE_WORKSPACE_NAME}::core
            ${CMAKE_WORKSPACE_NAME}::network
            ${CMAKE_WORKSPACE_NAME}::testing)
    endif()

    target_link_libraries(${target}
                          PRIVATE ${libraries})

    target_include_directories(${target}
                               PRIVATE
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${ACTOR_SOURCE_DIR}/src)

    set_target_properties(${target}
                          PROPERTIES
                          OUTPUT_NAME ${name}_perf)

    add_dependencies(perf_tests ${target})
    set(${name}_test ${target})
endmacro()

actor_add_test(rpc SOURCES rpc_perf.cc)