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

#include <nil/marshalling/endianness.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/status_type.hpp>

#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>
#include <nil/crypto3/marshalling/zk/types/commitments/eval_storage.hpp>
#include <nil/crypto3/marshalling/zk/types/commitments/lpc.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/common_data.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/preprocessed_public_data.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/proof.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/assignment_table.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/constraint_system.hpp>

#include <nil/crypto3/math/algorithms/calculate_domain_set.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/placeholder_policy.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/profiling.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/proof.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>
#include <nil/crypto3/zk/transcript/fiat_shamir.hpp>

#include <nil/blueprint/transpiler/recursive_verifier_generator.hpp>


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
                    BOOST_LOG_TRIVIAL(error) << "When reading a Marshalled structure from file " << path << ", decoding step failed";
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

            enum class ProverStage {
                ALL = 0,
                PREPROCESS = 1,
                PROVE = 2,
                VERIFY = 3,
                GENERATE_AGGREGATED_CHALLENGE = 4
            };

            ProverStage prover_stage_from_string(const std::string& stage) {
                static std::unordered_map<std::string, ProverStage> stage_map = {
                    {"all", ProverStage::ALL},
                    {"preprocess", ProverStage::PREPROCESS},
                    {"prove", ProverStage::PROVE},
                    {"verify", ProverStage::VERIFY},
                    {"generate-aggregated-challenge", ProverStage::GENERATE_AGGREGATED_CHALLENGE}
                };
                auto it = stage_map.find(stage);
                if (it == stage_map.end()) {
                    throw std::invalid_argument("Invalid stage: " + stage);
                }
                return it->second;
            }

        } // namespace detail


        template<typename CurveType, typename HashType>
        class Prover {
        public:
            using BlueprintField = typename CurveType::base_field_type;
            using LpcParams = nil::crypto3::zk::commitments::list_polynomial_commitment_params<HashType, HashType, 2>;
            using Lpc = nil::crypto3::zk::commitments::list_polynomial_commitment<BlueprintField, LpcParams>;
            using LpcScheme = typename nil::crypto3::zk::commitments::lpc_commitment_scheme<Lpc>;
            using CircuitParams = nil::crypto3::zk::snark::placeholder_circuit_params<BlueprintField>;
            using PlaceholderParams = nil::crypto3::zk::snark::placeholder_params<CircuitParams, LpcScheme>;
            using Proof = nil::crypto3::zk::snark::placeholder_proof<BlueprintField, PlaceholderParams>;
            using PublicPreprocessedData = typename nil::crypto3::zk::snark::
                placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using CommonData = typename PublicPreprocessedData::common_data_type;
            using PrivatePreprocessedData = typename nil::crypto3::zk::snark::
                placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::preprocessed_data_type;
            using ConstraintSystem = nil::crypto3::zk::snark::plonk_constraint_system<BlueprintField>;
            using TableDescription = nil::crypto3::zk::snark::plonk_table_description<BlueprintField>;
            using Endianness = nil::marshalling::option::big_endian;
            using FriParams = typename Lpc::fri_type::params_type;
            using Column = nil::crypto3::zk::snark::plonk_column<BlueprintField>;
            using AssignmentTable = nil::crypto3::zk::snark::plonk_table<BlueprintField, Column>;
            using TTypeBase = nil::marshalling::field_type<Endianness>;

            Prover(
                std::size_t lambda,
                std::size_t expand_factor,
                std::size_t max_q_chunks,
                std::size_t grind
            )
                : lambda_(lambda)
                , expand_factor_(expand_factor)
                , max_quotient_chunks_(max_q_chunks)
                , grind_(grind) {
            }

            // The caller must call the preprocessor or load the preprocessed data before calling this function.
            bool generate_to_file(
                    boost::filesystem::path proof_file_,
                    boost::filesystem::path json_file_,
                    bool skip_verification) {
                if (!nil::proof_generator::can_write_to_file(proof_file_.string())) {
                    BOOST_LOG_TRIVIAL(error) << "Can't write to file " << proof_file_;
                    return false;
                }

                BOOST_ASSERT(public_preprocessed_data_);
                BOOST_ASSERT(private_preprocessed_data_);
                BOOST_ASSERT(table_description_);
                BOOST_ASSERT(constraint_system_);
                BOOST_ASSERT(lpc_scheme_);

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
                    nil::crypto3::marshalling::types::fill_placeholder_proof<Endianness, Proof>(proof, lpc_scheme_->get_fri_params());
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    proof_file_,
                    filled_placeholder_proof,
                    true
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof written.";
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Failed to write proof to file.";
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

            bool verify_from_file(boost::filesystem::path proof_file_) {
                create_lpc_scheme();

                using ProofMarshalling = nil::crypto3::marshalling::types::
                    placeholder_proof<nil::marshalling::field_type<Endianness>, Proof>;

                BOOST_LOG_TRIVIAL(info) << "Reading proof from file";
                auto marshalled_proof = detail::decode_marshalling_from_file<ProofMarshalling>(proof_file_, true);
                if (!marshalled_proof) {
                    return false;
                }
                bool res = verify(nil::crypto3::marshalling::types::make_placeholder_proof<Endianness, Proof>(
                    *marshalled_proof));
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Proof verification passed.";
                }
                return res;
            }

            bool save_preprocessed_common_data_to_file(boost::filesystem::path preprocessed_common_data_file) {
                BOOST_LOG_TRIVIAL(info) << "Writing preprocessed common data to " << preprocessed_common_data_file << std::endl;
                auto marshalled_common_data =
                    nil::crypto3::marshalling::types::fill_placeholder_common_data<Endianness, CommonData>(
                        public_preprocessed_data_->common_data
                    );
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    preprocessed_common_data_file,
                    marshalled_common_data
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Preprocessed common data written.";
                }
                return res;
            }

            bool read_preprocessed_common_data_from_file(boost::filesystem::path preprocessed_common_data_file) {
                BOOST_LOG_TRIVIAL(info) << "Read preprocessed common data from " << preprocessed_common_data_file << std::endl;

                using CommonDataMarshalling = nil::crypto3::marshalling::types::placeholder_common_data<TTypeBase, CommonData>;

                auto marshalled_value = detail::decode_marshalling_from_file<CommonDataMarshalling>(
                    preprocessed_common_data_file);

                if (!marshalled_value) {
                    return false;
                }
                common_data_.emplace(nil::crypto3::marshalling::types::make_placeholder_common_data<Endianness, CommonData>(
                    *marshalled_value));

                return true;
            }

            // This includes not only the common data, but also merkle trees, polynomials, etc, everything that a
            // public preprocessor generates.
            bool save_public_preprocessed_data_to_file(boost::filesystem::path preprocessed_data_file) {
                using namespace nil::crypto3::marshalling::types;

                BOOST_LOG_TRIVIAL(info) << "Writing all preprocessed public data to " <<
                    preprocessed_data_file << std::endl;
                using PreprocessedPublicDataType = typename PublicPreprocessedData::preprocessed_data_type;

                auto marshalled_preprocessed_public_data =
                    fill_placeholder_preprocessed_public_data<Endianness, PreprocessedPublicDataType>(
                        *public_preprocessed_data_
                    );
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    preprocessed_data_file,
                    marshalled_preprocessed_public_data
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Preprocessed public data written.";
                }
                return res;
            }

            bool read_public_preprocessed_data_from_file(boost::filesystem::path preprocessed_data_file) {
                BOOST_LOG_TRIVIAL(info) << "Read preprocessed data from " << preprocessed_data_file << std::endl;

                using namespace nil::crypto3::marshalling::types;

                using PreprocessedPublicDataType = typename PublicPreprocessedData::preprocessed_data_type;
                using PublicPreprocessedDataMarshalling =
                    placeholder_preprocessed_public_data<TTypeBase, PreprocessedPublicDataType>;

                auto marshalled_value = detail::decode_marshalling_from_file<PublicPreprocessedDataMarshalling>(
                    preprocessed_data_file);
                if (!marshalled_value) {
                    return false;
                }
                public_preprocessed_data_.emplace(
                    make_placeholder_preprocessed_public_data<Endianness, PreprocessedPublicDataType>(*marshalled_value)
                );
                return true;
            }

            bool save_commitment_state_to_file(boost::filesystem::path commitment_scheme_state_file) {
                using namespace nil::crypto3::marshalling::types;

                BOOST_LOG_TRIVIAL(info) << "Writing commitment_state to " <<
                    commitment_scheme_state_file << std::endl;

                auto marshalled_lpc_state = fill_commitment_scheme<Endianness, LpcScheme>(
                    *lpc_scheme_);
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    commitment_scheme_state_file,
                    marshalled_lpc_state
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Commitment scheme written.";
                }
                return res;
            }

            bool read_commitment_scheme_from_file(boost::filesystem::path commitment_scheme_state_file) {
                BOOST_LOG_TRIVIAL(info) << "Read commitment scheme from " << commitment_scheme_state_file << std::endl;

                using namespace nil::crypto3::marshalling::types;

                using CommitmentStateMarshalling = typename commitment_scheme_state<TTypeBase, LpcScheme>::type;

                auto marshalled_value = detail::decode_marshalling_from_file<CommitmentStateMarshalling>(
                    commitment_scheme_state_file);
                if (!marshalled_value) {
                    return false;
                }
                lpc_scheme_.emplace(make_commitment_scheme<Endianness, LpcScheme>(*marshalled_value));
                return true;
            }

            bool verify(const Proof& proof) const {
                BOOST_LOG_TRIVIAL(info) << "Verifying proof...";
                bool verification_result =
                    nil::crypto3::zk::snark::placeholder_verifier<BlueprintField, PlaceholderParams>::process(
                        public_preprocessed_data_.has_value() ? public_preprocessed_data_->common_data : *common_data_,
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

            bool read_circuit(const boost::filesystem::path& circuit_file_) {
                BOOST_LOG_TRIVIAL(info) << "Read circuit from " << circuit_file_ << std::endl;

                using ConstraintMarshalling =
                    nil::crypto3::marshalling::types::plonk_constraint_system<TTypeBase, ConstraintSystem>;

                auto marshalled_value = detail::decode_marshalling_from_file<ConstraintMarshalling>(circuit_file_);
                if (!marshalled_value) {
                    return false;
                }
                constraint_system_.emplace(
                    nil::crypto3::marshalling::types::make_plonk_constraint_system<Endianness, ConstraintSystem>(
                        *marshalled_value
                    )
                );
                return true;
            }

            bool read_assignment_table(const boost::filesystem::path& assignment_table_file_) {
                BOOST_LOG_TRIVIAL(info) << "Read assignment table from " << assignment_table_file_ << std::endl;

                using TableValueMarshalling =
                    nil::crypto3::marshalling::types::plonk_assignment_table<TTypeBase, AssignmentTable>;
                auto marshalled_table =
                    detail::decode_marshalling_from_file<TableValueMarshalling>(assignment_table_file_);
                if (!marshalled_table) {
                    return false;
                }
                auto [table_description, assignment_table] =
                    nil::crypto3::marshalling::types::make_assignment_table<Endianness, AssignmentTable>(
                        *marshalled_table
                    );
                table_description_.emplace(table_description);
                assignment_table_.emplace(std::move(assignment_table));
                return true;
            }

            bool save_assignment_description(const boost::filesystem::path& assignment_description_file) {
                BOOST_LOG_TRIVIAL(info) << "Writing assignment description to " << assignment_description_file << std::endl;

                auto marshalled_assignment_description =
                    nil::crypto3::marshalling::types::fill_assignment_table_description<Endianness, BlueprintField>(
                        *table_description_
                    );
                bool res = nil::proof_generator::detail::encode_marshalling_to_file(
                    assignment_description_file,
                    marshalled_assignment_description
                );
                if (res) {
                    BOOST_LOG_TRIVIAL(info) << "Assignment description written.";
                }
                return res;
            }

            bool read_assignment_description(const boost::filesystem::path& assignment_description_file_) {
                BOOST_LOG_TRIVIAL(info) << "Read assignment description from " << assignment_description_file_ << std::endl;

                using TableDescriptionMarshalling =
                    nil::crypto3::marshalling::types::plonk_assignment_table_description<TTypeBase>;
                auto marshalled_description =
                    detail::decode_marshalling_from_file<TableDescriptionMarshalling>(assignment_description_file_);
                if (!marshalled_description) {
                    return false;
                }
                auto table_description =
                    nil::crypto3::marshalling::types::make_assignment_table_description<Endianness, BlueprintField>(
                        *marshalled_description
                    );
                table_description_.emplace(table_description);
                return true;
            }

            void create_lpc_scheme() {
                // Lambdas and grinding bits should be passed through preprocessor directives
                std::size_t table_rows_log = std::ceil(std::log2(table_description_->rows_amount));

                lpc_scheme_.emplace(FriParams(1, table_rows_log, lambda_, expand_factor_));
            }

            bool preprocess_public_data() {
                public_inputs_.emplace(assignment_table_->public_inputs());

                create_lpc_scheme();

                BOOST_LOG_TRIVIAL(info) << "Preprocessing public data";
                public_preprocessed_data_.emplace(
                    nil::crypto3::zk::snark::placeholder_public_preprocessor<BlueprintField, PlaceholderParams>::
                        process(
                            *constraint_system_,
                            assignment_table_->move_public_table(),
                            *table_description_,
                            *lpc_scheme_,
                            max_quotient_chunks_
                        )
                );
                return true;
            }

            bool preprocess_private_data() {

                BOOST_LOG_TRIVIAL(info) << "Preprocessing private data";
                private_preprocessed_data_.emplace(
                    nil::crypto3::zk::snark::placeholder_private_preprocessor<BlueprintField, PlaceholderParams>::
                        process(*constraint_system_, assignment_table_->move_private_table(), *table_description_)
                );

                // This is the last stage of preprocessor, and the assignment table is not used after this function call.
                assignment_table_.reset();

                return true;
            }

            bool generate_aggregated_challenge_to_file(
                const std::vector<boost::filesystem::path> &aggregate_input_files,
                const boost::filesystem::path &aggregated_challenge_file
            ) {
                if (aggregate_input_files.empty()) {
                    BOOST_LOG_TRIVIAL(error) << "No input files for challenge aggregation";
                    return false;
                }
                BOOST_LOG_TRIVIAL(info) << "Generating aggregated challenge to " << aggregated_challenge_file;
                // check that we can access all input files
                for (const auto &input_file : aggregate_input_files) {
                    BOOST_LOG_TRIVIAL(info) << "Reading challenge from " << input_file;
                    if (!nil::proof_generator::can_read_from_file(input_file.string())) {
                        BOOST_LOG_TRIVIAL(error) << "Can't read file " << input_file;
                        return false;
                    }
                }
                // create the transcript
                using transcript_hash_type = typename PlaceholderParams::transcript_hash_type;
                using transcript_type = crypto3::zk::transcript::fiat_shamir_heuristic_sequential<transcript_hash_type>;
                using challenge_marshalling_type =
                    nil::crypto3::marshalling::types::field_element<
                        TTypeBase, typename BlueprintField::value_type>;
                transcript_type transcript;
                // read challenges from input files and add them to the transcript
                for (const auto &input_file : aggregate_input_files) {
                    auto challenge = detail::decode_marshalling_from_file<challenge_marshalling_type>(input_file);
                    if (!challenge) {
                        BOOST_LOG_TRIVIAL(error) << "Failed to read challenge from " << input_file;
                        return false;
                    }
                    transcript(challenge->value());
                }
                // produce the aggregated challenge
                auto output_challenge = transcript.template challenge<BlueprintField>();
                // marshall the challenge
                challenge_marshalling_type marshalled_challenge(output_challenge);
                // write the challenge to the output file
                BOOST_LOG_TRIVIAL(info) << "Writing aggregated challenge to " << aggregated_challenge_file;
                return detail::encode_marshalling_to_file<challenge_marshalling_type>
                    (aggregated_challenge_file, marshalled_challenge);
            }

        private:
            const std::size_t expand_factor_;
            const std::size_t max_quotient_chunks_;
            const std::size_t lambda_;
            const std::size_t grind_;

            std::optional<PublicPreprocessedData> public_preprocessed_data_;

            // TODO: This is used in verifier, since it does not need the whole preprocessed data.
            // It makes sence to separate prover class from verifier later.
            std::optional<CommonData> common_data_;

            std::optional<PrivatePreprocessedData> private_preprocessed_data_;
            std::optional<typename AssignmentTable::public_input_container_type> public_inputs_;
            std::optional<TableDescription> table_description_;
            std::optional<ConstraintSystem> constraint_system_;
            std::optional<AssignmentTable> assignment_table_;
            std::optional<LpcScheme> lpc_scheme_;
        };

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_ASSIGNER_PROOF_HPP
