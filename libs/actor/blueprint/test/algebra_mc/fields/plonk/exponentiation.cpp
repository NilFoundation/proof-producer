//---------------------------------------------------------------------------//
// Copyright (c) 2022 Ilia Shirobokov <i.shirobokov@nil.foundation>
// Copyright (c) 2022 Ekaterina Chukavina <kate@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/sha2.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/actor/zk/snark/arithmetization/plonk/params.hpp>

#include <nil/actor_blueprint_mc/components/algebra/fields/plonk/exponentiation.hpp>
#include <nil/actor_blueprint_mc/blueprint/plonk.hpp>
#include <nil/actor_blueprint_mc/assignment/plonk.hpp>
#include "../../../test_plonk_component_mc.hpp"

using namespace nil;
using namespace nil::crypto3;

ACTOR_THREAD_TEST_CASE(blueprint_plonk_exponentiation) {
    auto start = std::chrono::high_resolution_clock::now();

    using curve_type = algebra::curves::pallas;
    using BlueprintFieldType = typename curve_type::base_field_type;
    constexpr std::size_t WitnessColumns = 15;
    constexpr std::size_t PublicInputColumns = 1;
    constexpr std::size_t ConstantColumns = 1;
    constexpr std::size_t SelectorColumns = 1;
    constexpr std::size_t n = 11;
    using ArithmetizationParams =
        actor::zk::snark::plonk_arithmetization_params<WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns>;
    using ArithmetizationType = actor::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>;
    using AssignmentType = nil::actor_blueprint_mc::blueprint_assignment_table<ArithmetizationType>;
    using hash_type = nil::crypto3::hashes::keccak_1600<256>;
    constexpr std::size_t Lambda = 1;

    using var = actor::zk::snark::plonk_variable<BlueprintFieldType>;

    using component_type = nil::actor_blueprint_mc::components::exponentiation<ArithmetizationType, n, 0, 1, 2, 3,
                                                                          4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>;
    var base(0, 0, false, var::column_type::public_input);
    var exponent(0, 1, false, var::column_type::public_input);


    typename BlueprintFieldType::value_type base_value = algebra::random_element<BlueprintFieldType>();
    typename BlueprintFieldType::value_type exponent_value = 654;
    typename BlueprintFieldType::integral_type exponent_value_integral = typename BlueprintFieldType::integral_type(exponent_value.data);

    std::vector<typename BlueprintFieldType::value_type> public_input = {base_value, exponent_value};

    typename component_type::params_type params = {base, exponent};

    typename BlueprintFieldType::value_type expected_result = power(base_value, exponent_value_integral);


    auto result_check = [&expected_result](AssignmentType &assignment,
        component_type::result_type &real_res) { 
        assert(expected_result== assignment.var_value(real_res.output));
    };

    nil::actor_blueprint_mc::test_component<component_type, BlueprintFieldType, ArithmetizationParams, hash_type, Lambda>(params, public_input, result_check);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
    std::cout << "exponentiation_component: " << duration.count() << "ms" << std::endl;
}

ACTOR_THREAD_TEST_CASE(blueprint_plonk_exponentiation_2) {
    auto start = std::chrono::high_resolution_clock::now();

    using curve_type = algebra::curves::pallas;
    using BlueprintFieldType = typename curve_type::base_field_type;
    constexpr std::size_t WitnessColumns = 15;
    constexpr std::size_t PublicInputColumns = 1;
    constexpr std::size_t ConstantColumns = 1;
    constexpr std::size_t SelectorColumns = 1;
    constexpr std::size_t n = 255;
    using ArithmetizationParams =
        actor::zk::snark::plonk_arithmetization_params<WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns>;
    using ArithmetizationType = actor::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>;
    using AssignmentType = nil::actor_blueprint_mc::blueprint_assignment_table<ArithmetizationType>;
    using hash_type = nil::crypto3::hashes::keccak_1600<256>;
    constexpr std::size_t Lambda = 1;

    using var = actor::zk::snark::plonk_variable<BlueprintFieldType>;

    using component_type = nil::actor_blueprint_mc::components::exponentiation<ArithmetizationType, n, 0, 1, 2, 3,
                                                                          4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>;
    var base(0, 0, false, var::column_type::public_input);
    var exponent(0, 1, false, var::column_type::public_input);


    typename BlueprintFieldType::value_type base_value = algebra::random_element<BlueprintFieldType>();
    typename BlueprintFieldType::value_type exponent_value = (BlueprintFieldType::value_type::modulus - 1) / 2;
    typename BlueprintFieldType::integral_type exponent_value_integral = typename BlueprintFieldType::integral_type(exponent_value.data);

    std::vector<typename BlueprintFieldType::value_type> public_input = {base_value, exponent_value};

    typename component_type::params_type params = {base, exponent};

    typename BlueprintFieldType::value_type expected_result = power(base_value, exponent_value_integral);


    auto result_check = [&expected_result](AssignmentType &assignment,
        component_type::result_type &real_res) { 
        assert(expected_result== assignment.var_value(real_res.output));
    };

    nil::actor_blueprint_mc::test_component<component_type, BlueprintFieldType, ArithmetizationParams, hash_type, Lambda>(params, public_input, result_check);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
    std::cout << "exponentiation_component: " << duration.count() << "ms" << std::endl;
}
