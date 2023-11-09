#---------------------------------------------------------------------------#
# Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
# Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
#
# Distributed under the Boost Software License, Version 1.0
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt
#---------------------------------------------------------------------------#

macro(define_blueprint_example name)
    add_executable(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}_${name}_example ${name}.cpp)
    target_link_libraries(blueprint_${name}_example PRIVATE
            ${CMAKE_WORKSPACE_NAME}_blueprint

            crypto3::algebra
            crypto3::math
            crypto3::block
            crypto3::hash
            crypto3::multiprecision
            crypto3::zk
            marshalling::crypto3_zk

            ${Boost_LIBRARIES})
    set_target_properties(blueprint_${name}_example PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED TRUE)
endmacro()


set(EXAMPLES_NAMES
        "curves"
        "test_component"
        "plonk/addition_component")

foreach (EXAMPLE_NAME ${EXAMPLES_NAMES})
    define_blueprint_example(${EXAMPLE_NAME})
endforeach ()