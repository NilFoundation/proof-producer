#---------------------------------------------------------------------------#
# Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
# Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@gmail.com>
#
# Distributed under the Boost Software License, Version 1.0
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt
#---------------------------------------------------------------------------#

include(CMTest)

cm_find_package(Boost REQUIRED COMPONENTS random unit_test_framework)

cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}

        crypto3::algebra
        crypto3::hash
        ${CMAKE_WORKSPACE_NAME}::math
        crypto3::multiprecision
        crypto3::random
        ${CMAKE_WORKSPACE_NAME}::zk
        marshalling::crypto3_zk

        ${Boost_LIBRARIES})

function(actor_add_test test)
    string(REPLACE "/" "_" full_test_name ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}_${test}_test)

    set(test_kinds
            ACTOR
            BOOST
            CUSTOM)

    cmake_parse_arguments(parsed_args
            ""
            "WORKING_DIRECTORY;KIND"
            "RUN_ARGS;SOURCES;LIBRARIES;DEPENDS"
            ${ARGN})

    if (NOT parsed_args_KIND)
        set(parsed_args_KIND ACTOR)
    elseif (NOT (parsed_args_KIND IN_LIST test_kinds))
        message(FATAL_ERROR "Invalid test kind. KIND must be one of ${test_kinds}")
    endif ()

    #
    # Each kind of test must populate the `args` and `libraries` lists.
    #

    set(libraries "${parsed_args_LIBRARIES}")
    list(APPEND libraries
            ${CMAKE_WORKSPACE_NAME}::core
            ${CMAKE_WORKSPACE_NAME}::network
            ${CMAKE_WORKSPACE_NAME}::testing
            ${CMAKE_WORKSPACE_NAME}::containers
            ${CMAKE_WORKSPACE_NAME}::math
            #               epoll-shim

            crypto3::algebra
            crypto3::hash
            ${CMAKE_WORKSPACE_NAME}::math
            crypto3::multiprecision
            crypto3::random
            ${CMAKE_WORKSPACE_NAME}::zk
            marshalling::crypto3_zk

            ${Boost_LIBRARIES})


    set(args "")
    if (parsed_args_KIND STREQUAL "ACTOR")
        list(APPEND args -- -c ${BUILD_WITH_UNIT_TEST_SMP})
    endif ()

    if (NOT (UNIX AND (CMAKE_SYSTEM_NAME STREQUAL "Linux")))
        list(APPEND libraries epoll-shim)
    endif ()

    list(APPEND args ${parsed_args_RUN_ARGS})

    cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
            ${libraries}
            ${Boost_LIBRARIES})

    cm_test(NAME ${full_test_name} SOURCES ${test}.cpp)

    target_compile_definitions(${full_test_name} PRIVATE
            ACTOR_TESTING_MAIN BOOST_TEST_DYN_LINK)

    if ((BUILD_WITH_STACK_GUARDS STREQUAL "ON") OR
    ((BUILD_WITH_STACK_GUARDS STREQUAL "DEFAULT") AND
    (CMAKE_BUILD_TYPE IN_LIST ACTOR_STACK_GUARD_MODES)))
        target_compile_definitions(${target_name}
                PRIVATE ACTOR_THREAD_STACK_GUARDS)
    endif ()

    target_include_directories(${full_test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${BUILD_WITH_SOURCE_DIR}/src)

    set_target_properties(${full_test_name} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED TRUE)

    target_compile_options(${full_test_name} PRIVATE
            ${${CURRENT_PROJECT_NAME}_PRIVATE_CXX_FLAGS})

    get_target_property(target_type Boost::unit_test_framework TYPE)
    if (target_type STREQUAL "SHARED_LIB")
        target_compile_definitions(${full_test_name} PRIVATE BOOST_TEST_DYN_LINK)
    elseif (target_type STREQUAL "STATIC_LIB")

    endif ()

endfunction()

macro(define_blueprint_test test)
    string(REPLACE "/" "_" full_test_name ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}_blueprint_${test}_test)

    cm_test(NAME ${full_test_name} SOURCES ${test}.cpp)

    target_include_directories(${full_test_name} PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>"

            ${Boost_INCLUDE_DIRS})

    set_target_properties(${full_test_name} PROPERTIES CXX_STANDARD 17)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${full_test_name} PRIVATE "-fconstexpr-steps=2147483647")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${full_test_name} PRIVATE "-fconstexpr-ops-limit=4294967295")
    endif ()

    get_target_property(target_type Boost::unit_test_framework TYPE)
    if (target_type STREQUAL "SHARED_LIB")
        target_compile_definitions(${full_test_name} PRIVATE BOOST_TEST_DYN_LINK)
    elseif (target_type STREQUAL "STATIC_LIB")

    endif ()
endmacro()

set(BLUEPRINT_MC_TESTS_FILES
    "algebra_mc/curves/plonk/decomposed_variable_base_scalar_mul"
    "algebra_mc/curves/plonk/endo_scalar"
    "algebra_mc/curves/plonk/unified_addition"
    "algebra_mc/curves/plonk/variable_base_scalar_mul"
    "algebra_mc/fields/plonk/combined_inner_product"
    "algebra_mc/fields/plonk/element_powers"
    "algebra_mc/fields/plonk/exponentiation"
    "algebra_mc/fields/plonk/range_check"
    "algebra_mc/fields/plonk/sqrt"
    )

set(NON_NATIVE_TESTS_FILES
    "algebra/fields/plonk/non_native/multiplication"
    "algebra/fields/plonk/non_native/addition"
    "algebra/fields/plonk/non_native/subtraction"
    "algebra/fields/plonk/non_native/range"
    "algebra/fields/plonk/non_native/reduction"
    "algebra/fields/plonk/non_native/bit_decomposition"
    "non_native/plonk/signatures_verification"
    "non_native/plonk/bool_scalar_multiplication"
    "non_native/plonk/scalar_non_native_range"
    )

set(PLONK_TESTS_FILES
    "algebra/curves/plonk/variable_base_scalar_mul"
    "algebra/curves/plonk/decomposed_variable_base_scalar_mul"
    "algebra/curves/plonk/unified_addition"
    "algebra/curves/plonk/variable_base_endo_scalar_mul"
    "algebra/curves/plonk/endo_scalar"
    "hashes/plonk/poseidon"
    "hashes/plonk/sha256"
    "hashes/plonk/sha512"
    "hashes/plonk/sha256_process"
    "hashes/plonk/sha512_process"
    "hashes/plonk/decomposition"
    "verifiers/kimchi/base_field"
    "verifiers/kimchi/prepare_batch_scalar"
    "verifiers/kimchi/verify_scalar"
    "verifiers/kimchi/basic_verifier"
    "verifiers/kimchi/table_commitment"
    "verifiers/kimchi/demo_verifier"
    "verifiers/kimchi/oracles_scalar"
    "verifiers/kimchi/batch_verify_base_field"
    "verifiers/kimchi/batch_verify_scalar_field"
    "verifiers/kimchi/detail/lagrange_denominators"
    "verifiers/kimchi/detail/b_poly"
    "verifiers/kimchi/detail/b_poly_coefficients"
    "verifiers/kimchi/detail/to_group"
    "verifiers/kimchi/detail/public_evaluations"
    "verifiers/kimchi/detail/prev_chal_evals"
    "verifiers/kimchi/detail/ft_eval"
    "verifiers/kimchi/detail/combine_proof_evals"
    "verifiers/kimchi/detail//constraints/index_terms_scalars"
    "verifiers/kimchi/detail/constraints/rpn_expression"
    "verifiers/kimchi/detail/constraints/vanishes_on_last_4_rows"
    "verifiers/kimchi/detail/constraints/unnormalized_lagrange_basis"
    "verifiers/kimchi/detail/constraints/perm_scalars"
    "verifiers/kimchi/detail/constraints/generic_scalars"
    "verifiers/kimchi/detail/oracles_cip"
    "verifiers/kimchi/detail/zk_w3"
    "verifiers/kimchi/detail/prepare_scalars"
    "verifiers/kimchi/detail/zkpm_evaluate"
    "verifiers/kimchi/sponge/sponge"
    "verifiers/kimchi/sponge/oracles"
    "verifiers/kimchi/sponge/transcript_fr"
    "verifiers/kimchi/sponge/transcript_fq"
    "verifiers/kimchi/sponge/compare"
    "algebra/fields/plonk/field_operations"
    "algebra/fields/plonk/combined_inner_product"
    "algebra/fields/plonk/element_powers"
    "algebra/fields/plonk/exponentiation"
    "algebra/fields/plonk/sqrt"
    "algebra/fields/plonk/range_check"
    "verifiers/pickles/verify_heterogenous_scalar"
    "verifiers/pickles/verify_heterogenous_base"
    "verifiers/pickles/scalar_details/evals_of_split_evals"
    )

set(FIELDS_TESTS_FILES
        "algebra/fields/r1cs/fp2"
        "algebra/fields/r1cs/fp2_verification"
        "algebra/fields/r1cs/fp3"
        "algebra/fields/r1cs/fp3_verification"
        "algebra/fields/r1cs/fp4"
        "algebra/fields/r1cs/fp4_verification"
        "algebra/fields/r1cs/fp6_2over3"
        "algebra/fields/r1cs/fp6_2over3_verification"
        #"algebra/fields/r1cs/exponentiation"
        )

set(CURVES_TESTS_FILES
        "algebra/curves/r1cs/montgomery"
        "algebra/curves/r1cs/twisted_edwards"
        "algebra/curves/r1cs/fixed_base_mul_zcash"
        )

set(HASHES_TESTS_FILES
        "hashes/r1cs/knapsack"
        "hashes/r1cs/knapsack_verification"
        "hashes/r1cs/sha256"
        "hashes/r1cs/sha256_verification"
        "hashes/r1cs/pedersen")

set(PAIRING_TESTS_FILES
        "algebra/pairing/weierstrass/r1cs/miller_loop"
        "algebra/pairing/weierstrass/r1cs/precomputation")

set(ROUTING_TESTS_FILES
        "routing_algorithms/routing_algorithms"
        "routing/r1cs/as_waksman"
        "routing/r1cs/benes")

set(SCHEMES_TESTS_FILES
        "verifiers/r1cs_ppzksnark"
        "set_commitment_component")

set(MERKLE_TREE_TESTS_FILES
        "merkle_tree_components")

set(VOTING_TESTS_FILES
        "voting/r1cs/encrypted_input_voting")

set(BASIC_COMPONENTS_TESTS_FILES
        "basic_components"
        "basic_components_r1cs_gg_ppzksnark")

SET(ALGEBRA_TESTS_FILES
        ${FIELDS_TESTS_FILES}
        ${CURVES_TESTS_FILES}
        ${PAIRING_TESTS_FILES})

SET(ALL_TESTS_FILES
        ${BLUEPRINT_MC_TESTS_FILES}
        ${NON_NATIVE_TESTS_FILES}
        ${PLONK_TESTS_FILES}
        ${ALGEBRA_TESTS_FILES}
        ${HASHES_TESTS_FILES}
        ${ROUTING_TESTS_FILES}
        ${SCHEMES_TESTS_FILES}
        ${MERKLE_TREE_TESTS_FILES}
        ${VOTING_TESTS_FILES}
        ${BASIC_COMPONENTS_TESTS_FILES})

foreach(TEST_FILE ${ALL_TESTS_FILES})
    actor_add_test(${TEST_FILE})
endforeach()

