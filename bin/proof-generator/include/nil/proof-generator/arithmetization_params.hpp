//---------------------------------------------------------------------------//
// Copyright (c) 2024 Iosif (x-mass) <x-mass@nil.foundation>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------//

#ifndef PROOF_GENERATOR_ARITHMETIZATION_PARAMS_HPP
#define PROOF_GENERATOR_ARITHMETIZATION_PARAMS_HPP

#include <array>
#include <tuple>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/crypto3/algebra/curves/vesta.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/hash/poseidon.hpp>
#include <nil/crypto3/hash/sha2.hpp>

#include <nil/proof-generator/non_type_arithmetization_params.hpp>

namespace nil {
    namespace proof_generator {

        using CurveTypes = std::tuple<nil::crypto3::algebra::curves::pallas
                                      // Add more curves as needed.
                                      >;

        using HashTypes = std::tuple<
            nil::crypto3::hashes::keccak_1600<256>,
            nil::crypto3::hashes::sha2<256>,
            nil::crypto3::hashes::poseidon<nil::crypto3::hashes::detail::mina_poseidon_policy<
                nil::crypto3::algebra::curves::pallas::base_field_type>>
            // Add more hashes as needed.
            >;

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_ARITHMETIZATION_PARAMS_HPP
