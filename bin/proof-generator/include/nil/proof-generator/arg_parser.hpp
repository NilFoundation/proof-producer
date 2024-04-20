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

#ifndef PROOF_GENERATOR_ARG_PARSER_HPP
#define PROOF_GENERATOR_ARG_PARSER_HPP

#include <optional>

#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <nil/proof-generator/arithmetization_params.hpp>
#include <nil/proof-generator/meta_utils.hpp>

namespace nil {
    namespace proof_generator {

        using CurvesVariant =
            typename tuple_to_variant<typename transform_tuple<CurveTypes, to_type_identity>::type>::type;
        using HashesVariant =
            typename tuple_to_variant<typename transform_tuple<HashTypes, to_type_identity>::type>::type;

        struct ProverOptions {
            boost::filesystem::path proof_file_path = "proof.bin";
            boost::filesystem::path json_file_path = "proof.json";
            boost::filesystem::path preprocessed_common_data_path = "preprocessed_common_data.dat";
            boost::filesystem::path circuit_file_path;
            boost::filesystem::path assignment_table_file_path;
            boost::log::trivial::severity_level log_level = boost::log::trivial::severity_level::info;
            bool skip_verification = false;
            bool verification_only = false;
            CurvesVariant elliptic_curve_type = type_identity<nil::crypto3::algebra::curves::pallas>{};
            HashesVariant hash_type = type_identity<nil::crypto3::hashes::keccak_1600<256>>{};
            ;
            std::size_t lambda = 9;
            std::size_t grind = 69;
            std::size_t expand_factor = 2;
        };

        std::optional<ProverOptions> parse_args(int argc, char* argv[]);

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_ARG_PARSER_HPP
