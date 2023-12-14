//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022-2023 Ilia Shirobokov <i.shirobokov@nil.foundation>
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

#include <sstream>
#include <fstream>
#include <random>

#include <boost/test/unit_test.hpp>

#include <nil/crypto3/algebra/curves/pallas.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/constraint_system.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/assignment_table.hpp>
#include <nil/crypto3/marshalling/zk/types/placeholder/proof.hpp>
#include <nil/crypto3/multiprecision/cpp_int.hpp>
#include <nil/crypto3/math/algorithms/calculate_domain_set.hpp>
#include <nil/crypto3/hash/keccak.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/params.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/preprocessor.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/prover.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/profiling.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/proof.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/verifier.hpp>
#include <nil/crypto3/zk/snark/systems/plonk/placeholder/detail/placeholder_policy.hpp>
#include <nil/crypto3/algebra/fields/arithmetic_params/pallas.hpp>

#include <nil/marshalling/status_type.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/endianness.hpp>

#include <nil/proof-generator/detail/utils.hpp>


namespace nil {
    namespace proof_generator {
        namespace detail {

            bool read_buffer_from_file(std::ifstream &ifile, std::vector<std::uint8_t> &v) {
                char c;
                char c1;
                uint8_t b;

                ifile >> c;
                if (c != '0')
                    return false;
                ifile >> c;
                if (c != 'x')
                    return false;
                while (ifile) {
                    std::string str = "";
                    ifile >> c >> c1;
                    if (!isxdigit(c) || !isxdigit(c1))
                        return false;
                    str += c;
                    str += c1;
                    b = stoi(str, 0, 0x10);
                    v.push_back(b);
                }
                return true;
            }

            inline std::vector<std::size_t> generate_random_step_list(const std::size_t r, const int max_step) {
                using dist_type = std::uniform_int_distribution<int>;
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
                        step_list.emplace_back(dist_type(1, max_step)(random_engine));
                        steps_sum += step_list.back();
                    }
                }
                return step_list;
            }

            template<typename FRIScheme, typename FieldType>
            typename FRIScheme::params_type create_fri_params(
                    std::size_t degree_log, const int max_step = 1, std::size_t expand_factor = 0) {
                std::size_t r = degree_log - 1;

                return typename FRIScheme::params_type(
                    (1 << degree_log) - 1, // max_degree
                    nil::crypto3::math::calculate_domain_set<FieldType>(degree_log + expand_factor, r),
                    generate_random_step_list(r, max_step),
                    expand_factor
                );
            }

        }    // namespace detail

        bool prover(boost::filesystem::path circuit_file_name, boost::filesystem::path assignment_table_file_name, boost::filesystem::path proof_file) {
            using curve_type = nil::crypto3::algebra::curves::pallas;
            using BlueprintFieldType = typename curve_type::base_field_type;
            constexpr std::size_t WitnessColumns = 15;
            constexpr std::size_t PublicInputColumns = 1;
            constexpr std::size_t ConstantColumns = 35;
            constexpr std::size_t SelectorColumns = 36;

            using ArithmetizationParams =
                nil::crypto3::zk::snark::plonk_arithmetization_params<WitnessColumns, PublicInputColumns, ConstantColumns,
                                                                    SelectorColumns>;
            using ConstraintSystemType =
                nil::crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>;
            using TableDescriptionType =
                nil::crypto3::zk::snark::plonk_table_description<BlueprintFieldType, ArithmetizationParams>;
            using Endianness = nil::marshalling::option::big_endian;
            using TTypeBase = nil::marshalling::field_type<Endianness>;
            using value_marshalling_type =
                nil::crypto3::marshalling::types::plonk_constraint_system<TTypeBase, ConstraintSystemType>;

            using ColumnType = nil::crypto3::zk::snark::plonk_column<BlueprintFieldType>;
            using AssignmentTableType =
                nil::crypto3::zk::snark::plonk_table<BlueprintFieldType, ArithmetizationParams, ColumnType>;
            using table_value_marshalling_type =
                nil::crypto3::marshalling::types::plonk_assignment_table<TTypeBase, AssignmentTableType>;

            using ColumnsRotationsType = std::array<std::set<int>, ArithmetizationParams::total_columns>;

            ConstraintSystemType constraint_system;
            {
                std::ifstream ifile;
                ifile.open(circuit_file_name);
                if (!ifile.is_open()) {
                    std::cout << "Cannot find input file " << circuit_file_name << std::endl;
                    return false;
                }
                std::vector<std::uint8_t> v;
                if (!proof_generator::detail::read_buffer_from_file(ifile, v)) {
                    std::cout << "Cannot parse input file " << circuit_file_name << std::endl;
                    return false;
                }
                ifile.close();

                value_marshalling_type marshalled_data;
                auto read_iter = v.begin();
                auto status = marshalled_data.read(read_iter, v.size());
                constraint_system = nil::crypto3::marshalling::types::make_plonk_constraint_system<Endianness, ConstraintSystemType>(
                        marshalled_data
                );
            }

            TableDescriptionType table_description;
            AssignmentTableType assignment_table;
            {
                std::ifstream iassignment;
                iassignment.open(assignment_table_file_name);
                if (!iassignment) {
                    std::cout << "Cannot open " << assignment_table_file_name << std::endl;
                    return false;
                }
                std::vector<std::uint8_t> v;
                if (!proof_generator::detail::read_buffer_from_file(iassignment, v)) {
                    std::cout << "Cannot parse input file " << assignment_table_file_name << std::endl;
                    return false;
                }
                iassignment.close();
                table_value_marshalling_type marshalled_table_data;
                auto read_iter = v.begin();
                auto status = marshalled_table_data.read(read_iter, v.size());
                std::tie(table_description.usable_rows_amount, assignment_table) =
                    nil::crypto3::marshalling::types::make_assignment_table<Endianness, AssignmentTableType>(
                        marshalled_table_data
                    );
                table_description.rows_amount = assignment_table.rows_amount();
            }

            const std::size_t Lambda = 9;
            using Hash = nil::crypto3::hashes::keccak_1600<256>;
            using circuit_params = nil::crypto3::zk::snark::placeholder_circuit_params<
                BlueprintFieldType, ArithmetizationParams
            >;

            std::size_t table_rows_log = std::ceil(std::log2(table_description.rows_amount));
            using lpc_params_type = nil::crypto3::zk::commitments::list_polynomial_commitment_params<
                Hash,
                Hash,
                Lambda,
                2
            >;
            using lpc_type = nil::crypto3::zk::commitments::list_polynomial_commitment<BlueprintFieldType, lpc_params_type>;
            using lpc_scheme_type = typename nil::crypto3::zk::commitments::lpc_commitment_scheme<lpc_type>;
            using placeholder_params = nil::crypto3::zk::snark::placeholder_params<circuit_params, lpc_scheme_type>;
            using policy_type = nil::crypto3::zk::snark::detail::placeholder_policy<BlueprintFieldType, placeholder_params>;

            auto fri_params = proof_generator::detail::create_fri_params<typename lpc_type::fri_type, BlueprintFieldType>(table_rows_log);
            std::size_t permutation_size =
                table_description.witness_columns + table_description.public_input_columns + table_description.constant_columns;
            lpc_scheme_type lpc_scheme(fri_params);

            std::cout << "Preprocessing public data..." << std::endl;
            typename nil::crypto3::zk::snark::placeholder_public_preprocessor<
                BlueprintFieldType, placeholder_params>::preprocessed_data_type public_preprocessed_data =
            nil::crypto3::zk::snark::placeholder_public_preprocessor<BlueprintFieldType, placeholder_params>::process(
                constraint_system, assignment_table.public_table(), table_description, lpc_scheme, permutation_size);

            std::cout << "Preprocessing private data..." << std::endl;
            typename nil::crypto3::zk::snark::placeholder_private_preprocessor<
                BlueprintFieldType, placeholder_params>::preprocessed_data_type private_preprocessed_data =
                nil::crypto3::zk::snark::placeholder_private_preprocessor<BlueprintFieldType, placeholder_params>::process(
                    constraint_system, assignment_table.private_table(), table_description
                );

            if (constraint_system.num_gates() == 0){
                std::cout << "Generating proof (zero gates)..." << std::endl;
                std::cout << "Proof generated" << std::endl;
                std::cout << "Writing proof to " << proof_file << "..." << std::endl;
                std::fstream fs;
                fs.open(proof_file, std::ios::out);
                fs.close();
                std::cout << "Proof written" << std::endl;
            } else {
                std::cout << "Generating proof..." << std::endl;
                using ProofType = nil::crypto3::zk::snark::placeholder_proof<BlueprintFieldType, placeholder_params>;
                ProofType proof = nil::crypto3::zk::snark::placeholder_prover<BlueprintFieldType, placeholder_params>::process(
                    public_preprocessed_data, private_preprocessed_data, table_description, constraint_system, assignment_table,
                    lpc_scheme);
                std::cout << "Proof generated" << std::endl;

                std::cout << "Verifying proof..." << std::endl;
                bool verification_result =
                    nil::crypto3::zk::snark::placeholder_verifier<BlueprintFieldType, placeholder_params>::process(
                        public_preprocessed_data, proof, constraint_system, lpc_scheme
                    );

                if (!verification_result) {
                    std::cout << "Something went wrong - proof is not verified" << std::endl;
                    return false;
                }

                std::cout << "Proof is verified" << std::endl;

                std::cout << "Writing proof to " << proof_file << "..." << std::endl;
                auto filled_placeholder_proof =
                    nil::crypto3::marshalling::types::fill_placeholder_proof<Endianness, ProofType>(proof);
                proof_print<Endianness, ProofType>(proof, proof_file);
                std::cout << "Proof written" << std::endl;
                return true;
            }
        }
    }        // namespace proof_generator
}    // namespace nil

#endif    // PROOF_GENERATOR_ASSIGNER_PROOF_HPP
