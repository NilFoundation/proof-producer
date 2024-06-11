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
#include <nil/crypto3/marshalling/zk/types/commitments/eval_storage.hpp>
#include <nil/crypto3/marshalling/zk/types/commitments/lpc.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/common_data.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/proof.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/assignment_table.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/constraint_system.hpp>
#include <nil/crypto3/math/algorithms/calculate_domain_set.hpp>
#include <nil/crypto3/multiprecision/cpp_int.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/placeholder_policy.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/profiling.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/proof.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>

#include <nil/blueprint/transpiler/recursive_verifier_generator.hpp>
#include <nil/marshalling/endianness.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/status_type.hpp>
#include <nil/proof-generator/arithmetization_params.hpp>
#include <nil/proof-generator/file_operations.hpp>

namespace nil {
    namespace proof_generator {
        namespace detail {
            template<typename MarshallingType>
            std::optional<MarshallingType> decode_marshalling_from_file(
                const boost::filesystem::path& path,
                bool hex = false
            ) {
                const auto v = hex ? read_hex_file_to_vector(path.c_str()) : read_file_to_vector(path.c_str());
                if (!v.has_value()) {
                    return std::nullopt;
                }

                MarshallingType marshalled_data;
                auto read_iter = v->begin();
                auto status = marshalled_data.read(read_iter, v->size());
                if (status != nil::marshalling::status_type::success) {
                    BOOST_LOG_TRIVIAL(error) << "Marshalled structure decoding failed";
                    return std::nullopt;
                }
                return marshalled_data;
            }


            template<typename MarshallingType>
            std::optional<MarshallingType> decode_table_from_file(
                const boost::filesystem::path& path
            ) {
                const auto v = read_table_file_to_vector(path.c_str());
                if (!v.has_value()) {
                    return std::nullopt;
                }

                MarshallingType marshalled_data;
                auto read_iter = v->begin();
                auto status = marshalled_data.read(read_iter, v->size());
                if (status != nil::marshalling::status_type::success) {
                    BOOST_LOG_TRIVIAL(error) << "Marshalled structure decoding failed";
                    return std::nullopt;
                }
                return marshalled_data;
            }

            template<typename MarshallingType>
            bool encode_marshalling_to_file(
                const boost::filesystem::path& path,
                const MarshallingType& data_for_marshalling,
                bool hex = false
            ) {
                std::vector<std::uint8_t> v;
                v.resize(data_for_marshalling.length(), 0x00);
                auto write_iter = v.begin();
                nil::marshalling::status_type status = data_for_marshalling.write(write_iter, v.size());
                if (status != nil::marshalling::status_type::success) {
                    BOOST_LOG_TRIVIAL(error) << "Marshalled structure encoding failed";
                    return false;
                }

                return hex ? write_vector_to_hex_file(v, path.c_str()) : write_vector_to_file(v, path.c_str());
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
        } // namespace detail

        template<typename CurveType, typename HashType>
        class Prover {
        public:
            Prover(
                boost::filesystem::path circuit_file_name,
                boost::filesystem::path preprocessed_common_data_file_name,
                boost::filesystem::path assignment_table_file_name,
                boost::filesystem::path proof_file,
                boost::filesystem::path json_file,
                std::size_t lambda,
                std::size_t expand_factor,
                std::size_t max_q_chunks,
                std::size_t grind
            )
                : circuit_file_(circuit_file_name)
                , preprocessed_common_data_file_(preprocessed_common_data_file_name)
                , assignment_table_file_(assignment_table_file_name)
                , proof_file_(proof_file)
                , json_file_(json_file)
                , lambda_(lambda)
                , expand_factor_(expand_factor)
                , max_quotient_chunks_(max_q_chunks)
                , grind_(grind) {
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
                Proof proof = nil::crypto3::zk::snark::placeholder_prover<BlueprintField, PlaceholderParams>::process(
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
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    proof_file_,
                    filled_placeholder_proof,
                    true
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof written";
                }

                BOOST_LOG_TRIVIAL(info) << "Writing json proof to " << json_file_;
                auto output_file = open_file<std::ofstream>(json_file_.string(), std::ios_base::out);
                if (!output_file)
                    return res;

                (*output_file) << nil::blueprint::recursive_verifier_generator<
                                      PlaceholderParams,
                                      nil::crypto3::zk::snark::placeholder_proof<BlueprintField, PlaceholderParams>,
                                      typename nil::crypto3::zk::snark::placeholder_public_preprocessor<
                                          BlueprintField,
                                          PlaceholderParams>::preprocessed_data_type::common_data_type>(
                                      *table_description_
                )
                                      .generate_input(*public_inputs_, proof, constraint_system_->public_input_sizes());
                output_file->close();

                return res;
            }

            bool verify_from_file() {
                prepare_for_operation();
                using ProofMarshalling = nil::crypto3::marshalling::types::
                    placeholder_proof<nil::marshalling::field_type<Endianness>, Proof>;
                BOOST_LOG_TRIVIAL(info) << "Reading proof from file";
                auto marshalled_proof = detail::decode_marshalling_from_file<ProofMarshalling>(proof_file_, true);
                if (!marshalled_proof) {
                    return false;
                }
                bool res =
                    verify(nil::crypto3::marshalling::types::make_placeholder_proof<Endianness, Proof>(*marshalled_proof
                    ));
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof verified";
                }
                return res;
            }

            bool save_preprocessed_common_data_to_file() {
                BOOST_LOG_TRIVIAL(info) << "Writing preprocessed common data to file...";
                using Endianness = nil::marshalling::option::big_endian;
                using TTypeBase = nil::marshalling::field_type<Endianness>;
                using CommonData = typename PublicPreprocessedData::preprocessed_data_type::common_data_type;
                auto marshalled_common_data =
                    nil::crypto3::marshalling::types::fill_placeholder_common_data<Endianness, CommonData>(
                        public_preprocessed_data_->common_data
                    );
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    preprocessed_common_data_file_,
                    marshalled_common_data
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Preprocessed common data written";
                }
                return res;
            }

        private:
            using BlueprintField = typename CurveType::base_field_type;
            using LpcParams = nil::crypto3::zk::commitments::list_polynomial_commitment_params<HashType, HashType, 2>;
            using Lpc = nil::crypto3::zk::commitments::list_polynomial_commitment<BlueprintField, LpcParams>;
            using LpcScheme = typename nil::crypto3::zk::commitments::lpc_commitment_scheme<Lpc>;
            using CircuitParams = nil::crypto3::zk::snark::placeholder_circuit_params<BlueprintField>;
            using PlaceholderParams = nil::crypto3::zk::snark::placeholder_params<CircuitParams, LpcScheme>;
            using Proof = nil::crypto3::zk::snark::placeholder_proof<BlueprintField, PlaceholderParams>;
            using PublicPreprocessedData = typename nil::crypto3::zk::snark::
                placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using PrivatePreprocessedData = typename nil::crypto3::zk::snark::
                placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using ConstraintSystem = nil::crypto3::zk::snark::plonk_constraint_system<BlueprintField>;
            using TableDescription = nil::crypto3::zk::snark::plonk_table_description<BlueprintField>;
            using Endianness = nil::marshalling::option::big_endian;
            using FriParams = typename Lpc::fri_type::params_type;
            using Column = nil::crypto3::zk::snark::plonk_column<BlueprintField>;
            using AssignmentTable = nil::crypto3::zk::snark::plonk_table<BlueprintField, Column>;

            bool verify(const Proof& proof) const {
                BOOST_LOG_TRIVIAL(info) << "Verifying proof...";
                bool verification_result =
                    nil::crypto3::zk::snark::placeholder_verifier<BlueprintField, PlaceholderParams>::process(
                        public_preprocessed_data_->common_data,
                        proof,
                        *table_description_,
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

                {
                    auto marshalled_value = detail::decode_marshalling_from_file<ConstraintMarshalling>(circuit_file_);
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
                auto marshalled_table =
                    detail::decode_table_from_file<TableValueMarshalling>(assignment_table_file_);
                if (!marshalled_table) {
                    return false;
                }
                auto [table_description, assignment_table] =
                    nil::crypto3::marshalling::types::make_assignment_table<Endianness, AssignmentTable>(
                        *marshalled_table
                    );

                public_inputs_.emplace(assignment_table.public_inputs());
                table_description_.emplace(table_description);

                // Lambdas and grinding bits should be passed threw preprocessor directives
                std::size_t table_rows_log = std::ceil(std::log2(table_description_->rows_amount));

                fri_params_.emplace(FriParams(1, table_rows_log, lambda_, expand_factor_));
                lpc_scheme_.emplace(*fri_params_);

                BOOST_LOG_TRIVIAL(info) << "Preprocessing public data";
                public_preprocessed_data_.emplace(
                    nil::crypto3::zk::snark::placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::
                        process(
                            *constraint_system_,
                            assignment_table.move_public_table(),
                            *table_description_,
                            *lpc_scheme_,
                            max_quotient_chunks_
                        )
                );

                BOOST_LOG_TRIVIAL(info) << "Preprocessing private data";
                private_preprocessed_data_.emplace(
                    nil::crypto3::zk::snark::placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::
                        process(*constraint_system_, assignment_table.move_private_table(), *table_description_)
                );
                return true;
            }

            const boost::filesystem::path circuit_file_;
            const boost::filesystem::path preprocessed_common_data_file_;
            const boost::filesystem::path assignment_table_file_;
            const boost::filesystem::path proof_file_;
            const boost::filesystem::path json_file_;
            const std::size_t expand_factor_;
            const std::size_t max_quotient_chunks_;
            const std::size_t lambda_;
            const std::size_t grind_;

            // All set on prepare_for_operation()
            std::optional<PublicPreprocessedData> public_preprocessed_data_;
            std::optional<PrivatePreprocessedData> private_preprocessed_data_;
            std::optional<typename AssignmentTable::public_input_container_type> public_inputs_;
            std::optional<TableDescription> table_description_;
            std::optional<ConstraintSystem> constraint_system_;
            std::optional<FriParams> fri_params_;
            std::optional<LpcScheme> lpc_scheme_;
        };

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_ASSIGNER_PROOF_HPP
