//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
// Copyright (c) 2022 Ilia Shirobokov <i.shirobokov@nil.foundation>
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

#include <iostream>
#include <optional>
#include <utility>

#include <nil/proof-generator/arg_parser.hpp>
#include <nil/proof-generator/file_operations.hpp>
#include <nil/proof-generator/prover.hpp>

#undef B0

using namespace nil::proof_generator;

template<typename CurveType, typename HashType>
int run_prover(const nil::proof_generator::ProverOptions& prover_options) {
    auto prover_task = [&] {
        auto prover = nil::proof_generator::Prover<CurveType, HashType>(
            prover_options.circuit_file_path,
            prover_options.preprocessed_common_data_path,
            prover_options.assignment_table_file_path,
            prover_options.proof_file_path,
            prover_options.json_file_path,
            prover_options.lambda,
            prover_options.expand_factor,
            prover_options.max_quotient_chunks,
            prover_options.grind
        );
        bool prover_result;
        try {
            prover_result = prover_options.verification_only ? prover.verify_from_file()
                                                             : prover.generate_to_file(prover_options.skip_verification)
                    && prover.save_preprocessed_common_data_to_file();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
            return 1;
        }
        return prover_result ? 0 : 1;
    };
    return prover_task();
}

// We could either make lambdas for generating Cartesian products of templates,
// but this would lead to callback hell. Instead, we declare extra function for
// each factor. Last declared function starts the chain.
template<typename CurveType>
int hash_wrapper(const ProverOptions& prover_options) {
    int ret;
    auto run_prover_wrapper_void = [&prover_options, &ret]<typename HashTypeIdentity>() {
        using HashType = typename HashTypeIdentity::type;
        ret = run_prover<CurveType, HashType>(prover_options);
    };
    pass_variant_type_to_template_func<HashesVariant>(prover_options.hash_type, run_prover_wrapper_void);
    return ret;
}

int curve_wrapper(const ProverOptions& prover_options) {
    int ret;
    auto curves_wrapper_void = [&prover_options, &ret]<typename CurveTypeIdentity>() {
        using CurveType = typename CurveTypeIdentity::type;
        ret = hash_wrapper<CurveType>(prover_options);
    };
    pass_variant_type_to_template_func<CurvesVariant>(prover_options.elliptic_curve_type, curves_wrapper_void);
    return ret;
}

int initial_wrapper(const ProverOptions& prover_options) {
    return curve_wrapper(prover_options);
}

int main(int argc, char* argv[]) {
    std::optional<nil::proof_generator::ProverOptions> prover_options = nil::proof_generator::parse_args(argc, argv);
    if (!prover_options) {
        // Action has already taken a place (help, version, etc.)
        return 0;
    }
    return initial_wrapper(*prover_options);
}
