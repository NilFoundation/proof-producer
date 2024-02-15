//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022-2023 Ilia Shirobokov <i.shirobokov@nil.foundation>
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

#ifndef PROOF_GENERATOR_ASSIGNER_PROOF_HPP
#define PROOF_GENERATOR_ASSIGNER_PROOF_HPP

#include <fstream>
#include <random>
#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp> // TODO: remove this. Required only because of an incorrect assert check in zk

#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/proof.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/assignment_table.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/constraint_system.hpp>
#include <nil/crypto3/multiprecision/cpp_int.hpp>

#ifdef PROOF_GENERATOR_MODE_MULTI_THREADED

#include <nil/actor/math/algorithms/calculate_domain_set.hpp>
#include <nil/actor/zk/snark/arithmetization/plonk/constraint_system.hpp>
#include <nil/actor/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/detail/placeholder_policy.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/profiling.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/proof.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/actor/zk/snark/systems/plonk/placeholder/verifier.hpp>

#else

#include <nil/crypto3/math/algorithms/calculate_domain_set.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/placeholder_policy.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/profiling.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/proof.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>

#endif

#include <nil/marshalling/endianness.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/status_type.hpp>
#include <nil/proof-generator/arithmetization_params.hpp>
#include <nil/proof-generator/detail/utils.hpp>
#include <nil/proof-generator/file_operations.hpp>

#ifdef PROOF_GENERATOR_MODE_MULTI_THREADED
namespace proof_gen = nil::actor;
#else
namespace proof_gen = nil::crypto3;
#endif

namespace nil {
    namespace proof_generator {
        namespace detail {
            template<typename MarshallingType>
            std::optional<MarshallingType> parse_marshalling_from_file(const boost::filesystem::path& path) {
                const auto v = read_file_to_vector(path.c_str());
                if (!v.has_value()) {
                    return std::nullopt;
                }

                MarshallingType marshalled_data;
                auto read_iter = v->begin();
                auto status = marshalled_data.read(read_iter, v->size());
                if (status != nil::marshalling::status_type::success) {
                    BOOST_LOG_TRIVIAL(error) << "Marshalled structure parsing failed";
                    return std::nullopt;
                }
                return marshalled_data;
            }

            std::vector<std::size_t> generate_random_step_list(const std::size_t r, const int max_step) {
                using Distribution = std::uniform_int_distribution<int>;
                static std::random_device random_engine;

                std::vector<std::size_t> step_list;
                std::size_t steps_sum = 0;
                while (steps_sum != r) {
                    if (r - steps_sum <= max_step) {
                        while (r - steps_sum != 1) {
                            step_list.emplace_back(r - steps_sum - 1);
                            steps_sum += step_list.back();
                        }
                        step_list.emplace_back(1);
                        steps_sum += step_list.back();
                    } else {
                        step_list.emplace_back(Distribution(1, max_step)(random_engine));
                        steps_sum += step_list.back();
                    }
                }
                return step_list;
            }

            template<typename FRIScheme, typename FieldType>
            typename FRIScheme::params_type create_fri_params(
                std::size_t degree_log,
                const int max_step = 1,
                std::size_t expand_factor = 0
            ) {
                std::size_t r = degree_log - 1;

                return typename FRIScheme::params_type(
                    (1 << degree_log) - 1, // max_degree
#ifdef PROOF_GENERATOR_MODE_MULTI_THREADED
                    nil::actor::math::calculate_domain_set<FieldType>(degree_log + expand_factor, r).get(),
#else
                    nil::crypto3::math::calculate_domain_set<FieldType>(degree_log + expand_factor, r),
#endif
                    generate_random_step_list(r, max_step),
                    expand_factor
                );
            }
        } // namespace detail

        template<
            typename CurveType,
            typename HashType,
            std::size_t ColumnsParamsIdx,
            std::size_t LambdaParamIdx,
            std::size_t GrindParamIdx>
        class Prover {
        public:
            Prover(
                boost::filesystem::path circuit_file_name,
                boost::filesystem::path assignment_table_file_name,
                boost::filesystem::path proof_file
            )
                : circuit_file_(circuit_file_name)
                , assignment_table_file_(assignment_table_file_name)
                , proof_file_(proof_file) {
            }

            bool generate_to_file(bool skip_verification) {
                if (!nil::proof_generator::can_write_to_file(proof_file_.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't write to file " << proof_file_;
                    return false;
                }

                prepare_for_operation();

                BOOST_ASSERT(public_preprocessed_data_);
                BOOST_ASSERT(private_preprocessed_data_);
                BOOST_ASSERT(table_description_);
                BOOST_ASSERT(constraint_system_);
                BOOST_ASSERT(lpc_scheme_);
                BOOST_ASSERT(fri_params_);

                BOOST_LOG_TRIVIAL(info) << "Generating proof...";
                Proof proof = proof_gen::zk::snark::placeholder_prover<BlueprintField, PlaceholderParams>::process(
                    *public_preprocessed_data_,
                    *private_preprocessed_data_,
                    *table_description_,
                    *constraint_system_,
                    *lpc_scheme_
                );
                BOOST_LOG_TRIVIAL(info) << "Proof generated";

                if (skip_verification) {
                    BOOST_LOG_TRIVIAL(info) << "Skipping proof verification";
                } else {
                    if (!verify(proof)) {
                        return false;
                    }
                }

                BOOST_LOG_TRIVIAL(info) << "Writing proof to " << proof_file_;
                auto filled_placeholder_proof =
                    nil::crypto3::marshalling::types::fill_placeholder_proof<Endianness, Proof>(proof, *fri_params_);
                nil::proof_generator::detail::proof_print<Endianness, Proof>(proof, *fri_params_, proof_file_);
                BOOST_LOG_TRIVIAL(info) << "Proof written";
                return true;
            }

            bool verify_from_file() {
                prepare_for_operation();
                using ProofMarshalling = nil::crypto3::marshalling::types::
                    placeholder_proof<nil::marshalling::field_type<Endianness>, Proof>;
                auto marshalled_proof = detail::parse_marshalling_from_file<ProofMarshalling>(proof_file_);
                if (!marshalled_proof) {
                    return false;
                }
                return verify(
                    nil::crypto3::marshalling::types::make_placeholder_proof<Endianness, Proof>(*marshalled_proof)
                );
            }

        private:
            // C++20 allows passing non-type template param by value, so we could make the
            // current function use
            //   columns_policy type directly instead of index after standard upgrade.
            // clang-format off
            static constexpr std::size_t WitnessColumns = all_columns_params[ColumnsParamsIdx].witness_columns;
            static constexpr std::size_t PublicInputColumns = all_columns_params[ColumnsParamsIdx].public_input_columns;
            static constexpr std::size_t ComponentConstantColumns = all_columns_params[ColumnsParamsIdx].component_constant_columns;
            static constexpr std::size_t LookupConstantColumns = all_columns_params[ColumnsParamsIdx].lookup_constant_columns;
            static constexpr std::size_t ConstantColumns = ComponentConstantColumns + LookupConstantColumns;
            static constexpr std::size_t ComponentSelectorColumns = all_columns_params[ColumnsParamsIdx].component_selector_columns;
            static constexpr std::size_t LookupSelectorColumns = all_columns_params[ColumnsParamsIdx].lookup_selector_columns;
            static constexpr std::size_t SelectorColumns = ComponentSelectorColumns + LookupSelectorColumns;
            // clang-format on

            using ArithmetizationParams = proof_gen::zk::snark::
                plonk_arithmetization_params<WitnessColumns, PublicInputColumns, ConstantColumns, SelectorColumns>;
            using BlueprintField = typename CurveType::base_field_type;
            using LpcParams = proof_gen::zk::commitments::
                list_polynomial_commitment_params<HashType, HashType, all_lambda_params[LambdaParamIdx], 2>;
            using Lpc = proof_gen::zk::commitments::list_polynomial_commitment<BlueprintField, LpcParams>;
            using LpcScheme = typename proof_gen::zk::commitments::lpc_commitment_scheme<Lpc>;
            using CircuitParams =
                proof_gen::zk::snark::placeholder_circuit_params<BlueprintField, ArithmetizationParams>;
            using PlaceholderParams = proof_gen::zk::snark::placeholder_params<CircuitParams, LpcScheme>;
            using Proof = proof_gen::zk::snark::placeholder_proof<BlueprintField, PlaceholderParams>;
            using PublicPreprocessedData = typename proof_gen::zk::snark::
                placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using PrivatePreprocessedData = typename proof_gen::zk::snark::
                placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using ConstraintSystem =
                proof_gen::zk::snark::plonk_constraint_system<BlueprintField, ArithmetizationParams>;
            using TableDescription =
                proof_gen::zk::snark::plonk_table_description<BlueprintField, ArithmetizationParams>;
            using Endianness = nil::marshalling::option::big_endian;
            using FriParams = typename Lpc::fri_type::params_type;

            bool verify(const Proof& proof) const {
                BOOST_LOG_TRIVIAL(info) << "Verifying proof...";
                bool verification_result =
                    proof_gen::zk::snark::placeholder_verifier<BlueprintField, PlaceholderParams>::process(
                        *public_preprocessed_data_,
                        proof,
                        *constraint_system_,
                        *lpc_scheme_
                    );

                if (verification_result) {
                    BOOST_LOG_TRIVIAL(info) << "Proof is verified";
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Proof verification failed";
                }

                return verification_result;
            }

            bool prepare_for_operation() {
                using BlueprintField = typename CurveType::base_field_type;
                using TTypeBase = nil::marshalling::field_type<Endianness>;
                using ConstraintMarshalling =
                    nil::crypto3::marshalling::types::plonk_constraint_system<TTypeBase, ConstraintSystem>;

                using Column = proof_gen::zk::snark::plonk_column<BlueprintField>;
                using AssignmentTable =
                    proof_gen::zk::snark::plonk_table<BlueprintField, ArithmetizationParams, Column>;

                {
                    auto marshalled_value = detail::parse_marshalling_from_file<ConstraintMarshalling>(circuit_file_);
                    if (!marshalled_value) {
                        return false;
                    }
                    constraint_system_.emplace(
                        nil::crypto3::marshalling::types::make_plonk_constraint_system<Endianness, ConstraintSystem>(
                            *marshalled_value
                        )
                    );
                }

                using TableValueMarshalling =
                    nil::crypto3::marshalling::types::plonk_assignment_table<TTypeBase, AssignmentTable>;
                AssignmentTable assignment_table;
                {
                    TableDescription table_description;
                    auto marshalled_table =
                        detail::parse_marshalling_from_file<TableValueMarshalling>(assignment_table_file_);
                    if (!marshalled_table) {
                        return false;
                    }
                    std::tie(table_description.usable_rows_amount, assignment_table) =
                        nil::crypto3::marshalling::types::make_assignment_table<Endianness, AssignmentTable>(
                            *marshalled_table
                        );
                    table_description.rows_amount = assignment_table.rows_amount();
                    table_description_.emplace(table_description);
                }

                // Lambdas and grinding bits should be passed threw preprocessor directives
                std::size_t table_rows_log = std::ceil(std::log2(table_description_->rows_amount));

                fri_params_.emplace(detail::create_fri_params<typename Lpc::fri_type, BlueprintField>(table_rows_log));

                std::size_t permutation_size = table_description_->witness_columns
                    + table_description_->public_input_columns + ComponentConstantColumns;
                lpc_scheme_.emplace(*fri_params_);

                BOOST_LOG_TRIVIAL(info) << "Preprocessing public data";
                public_preprocessed_data_.emplace(
                    proof_gen::zk::snark::placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::process(
                        *constraint_system_,
                        assignment_table.move_public_table(),
                        *table_description_,
                        *lpc_scheme_,
                        permutation_size
                    )
                );

                BOOST_LOG_TRIVIAL(info) << "Preprocessing private data";
                private_preprocessed_data_.emplace(
                    proof_gen::zk::snark::placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::process(
                        *constraint_system_,
                        assignment_table.move_private_table(),
                        *table_description_
                    )
                );
                return true;
            }

            const boost::filesystem::path circuit_file_;
            const boost::filesystem::path assignment_table_file_;
            const boost::filesystem::path proof_file_;

            // All set on prepare_for_operation()
            std::optional<PublicPreprocessedData> public_preprocessed_data_;
            std::optional<PrivatePreprocessedData> private_preprocessed_data_;
            std::optional<TableDescription> table_description_;
            std::optional<ConstraintSystem> constraint_system_;
            std::optional<FriParams> fri_params_;
            std::optional<LpcScheme> lpc_scheme_;
        };

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_ASSIGNER_PROOF_HPP
