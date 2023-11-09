//---------------------------------------------------------------------------//
// Copyright (c) 2020-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
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

#include <vector>
#include <cstdint>

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>

#include <nil/crypto3/algebra/fields/bls12/base_field.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/bls12.hpp>

#include <nil/actor/math/polynomial/basic_operations.hpp>
#include <nil/actor/math/kronecker_substitution.hpp>

using namespace nil::crypto3::algebra;
using namespace nil::actor::math;

typedef fields::bls12<381> FieldType;

ACTOR_THREAD_TEST_CASE(standard_polynomial_multiplication) {

    std::vector<typename FieldType::value_type> a = {1, 2, 3, 1};
    std::vector<typename FieldType::value_type> b = {1, 2, 1, 1};
    std::vector<typename FieldType::value_type> c(1, FieldType::value_type::zero());

    polynomial::multiplication_on_kronecker<FieldType>(c, a, b);

    std::vector<typename FieldType::value_type> c_answer(1, FieldType::value_type::zero());
    multiplication(c_answer, a, b).get();

    for (std::size_t i = 0; i < c_answer.size(); i++) {
        BOOST_CHECK_EQUAL(c_answer[i].data, c[i].data);
    }
}

ACTOR_THREAD_TEST_CASE(squared_polynomial_multiplication) {

    std::vector<typename FieldType::value_type> a = {1, 2, 3, 1};
    std::vector<typename FieldType::value_type> b = a;
    std::vector<typename FieldType::value_type> c(1, FieldType::value_type::zero());

    polynomial::multiplication_on_kronecker<FieldType>(c, a, b);

    std::vector<typename FieldType::value_type> c_answer(1, FieldType::value_type::zero());
    multiplication(c_answer, a, b).get();

    for (std::size_t i = 0; i < c_answer.size(); i++) {
        BOOST_CHECK_EQUAL(c_answer[i].data, c[i].data);
    }
}