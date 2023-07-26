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

#include <nil/crypto3/algebra/curves/pallas.hpp>

#include <nil/crypto3/zk/snark/arithmetization/plonk/params.hpp>
#include <nil/crypto3/zk/snark/arithmetization/plonk/constraint_system.hpp>

#include <nil/blueprint/transpiler/table_profiling.hpp>

#include <nil/marshalling/status_type.hpp>
#include <nil/marshalling/field_type.hpp>
#include <nil/marshalling/endianness.hpp>
#include <nil/crypto3/marshalling/zk/types/plonk/constraint_system.hpp>
#include <nil/crypto3/multiprecision/cpp_int.hpp>

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
            typename FRIScheme::params_type create_fri_params(std::size_t degree_log, const int max_step = 1) {
                typename FRIScheme::params_type params;
                nil::crypto3::math::polynomial<typename FieldType::value_type> q = {0, 0, 1};

                constexpr std::size_t expand_factor = 0;
                std::size_t r = degree_log - 1;

                std::vector<std::shared_ptr<nil::crypto3::math::evaluation_domain<FieldType>>> domain_set =
                    nil::crypto3::math::calculate_domain_set<FieldType>(degree_log + expand_factor, r);

                params.r = r;
                params.D = domain_set;
                params.max_degree = (1 << degree_log) - 1;
                params.step_list = generate_random_step_list(r, max_step);

                return params;
            }
        
        }    // namespace detail

        void prover(boost::filesystem::path circuit_file_path, boost::filesystem::path assignment_file_path, boost::filesystem::path proof_file) {
            using curve_type = nil::crypto3::algebra::curves::pallas;
            using BlueprintFieldType = typename curve_type::base_field_type;
            constexpr std::size_t WitnessColumns = 15;
            constexpr std::size_t PublicInputColumns = 5;
            constexpr std::size_t ConstantColumns = 5;
            constexpr std::size_t SelectorColumns = 30;

            using ArithmetizationParams =
                nil::crypto3::zk::snark::plonk_arithmetization_params<WitnessColumns, PublicInputColumns,
                                                                        ConstantColumns, SelectorColumns>;
            using ConstraintSystemType =
                nil::crypto3::zk::snark::plonk_constraint_system<BlueprintFieldType, ArithmetizationParams>;

            std::vector<typename BlueprintFieldType::value_type> public_input;

            std::cout << "generatring zkllvm proof..." << std::endl;

            std::ifstream ifile;
            ifile.open(circuit_file_path);
            if (!ifile.is_open()) {
                std::cout << "Cannot find input file " << circuit_file_path << std::endl;
                return;
            }
            std::vector<std::uint8_t> v;
            if (!detail::read_buffer_from_file(ifile, v)) {
                std::cout << "Cannot parse input file " << circuit_file_path << std::endl;
                return;
            }
            ifile.close();

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
            using ColumnsRotationsType = std::array<std::vector<int>, ArithmetizationParams::total_columns>;

            value_marshalling_type marshalled_data;
            TableDescriptionType table_description;
            auto read_iter = v.begin();
            auto status = marshalled_data.read(read_iter, v.size());
            auto constraint_system =
                nil::crypto3::marshalling::types::make_plonk_constraint_system<ConstraintSystemType, Endianness>(
                    marshalled_data);

            using ColumnType = nil::crypto3::zk::snark::plonk_column<BlueprintFieldType>;
            using AssignmentTableType =
                nil::crypto3::zk::snark::plonk_table<BlueprintFieldType, ArithmetizationParams, ColumnType>;

            std::ifstream iassignment;
            iassignment.open(assignment_file_path);
            if (!iassignment) {
                std::cout << "Cannot open " << assignment_file_path << std::endl;
                return;
            }
            AssignmentTableType assignment_table;
            std::tie(table_description.usable_rows_amount, table_description.rows_amount, assignment_table) =
                nil::blueprint::load_assignment_table<BlueprintFieldType, ArithmetizationParams, ColumnType>(iassignment);

            const std::size_t Lambda = 2;
            using Hash = nil::crypto3::hashes::keccak_1600<256>;
            using placeholder_params =
                nil::crypto3::zk::snark::placeholder_params<BlueprintFieldType, ArithmetizationParams, Hash, Hash, Lambda>;
            using types = nil::crypto3::zk::snark::detail::placeholder_policy<BlueprintFieldType, placeholder_params>;

            using FRIScheme =
                typename nil::crypto3::zk::commitments::fri<BlueprintFieldType, typename placeholder_params::merkle_hash_type,
                                                            typename placeholder_params::transcript_hash_type, Lambda, 2, 4>;
            using FRIParamsType = typename FRIScheme::params_type;

            std::size_t table_rows_log = std::ceil(std::log2(table_description.rows_amount));
            auto fri_params = detail::create_fri_params<FRIScheme, BlueprintFieldType>(table_rows_log);
            std::size_t permutation_size =
                table_description.witness_columns + table_description.public_input_columns + table_description.constant_columns;

            typename nil::crypto3::zk::snark::placeholder_public_preprocessor<
                BlueprintFieldType, placeholder_params>::preprocessed_data_type public_preprocessed_data =
                nil::crypto3::zk::snark::placeholder_public_preprocessor<BlueprintFieldType, placeholder_params>::process(
                    constraint_system, assignment_table.public_table(), table_description, fri_params, permutation_size);
            typename nil::crypto3::zk::snark::placeholder_private_preprocessor<
                BlueprintFieldType, placeholder_params>::preprocessed_data_type private_preprocessed_data =
                nil::crypto3::zk::snark::placeholder_private_preprocessor<BlueprintFieldType, placeholder_params>::process(
                    constraint_system, assignment_table.private_table(), table_description, fri_params
                );

            using ProofType = nil::crypto3::zk::snark::placeholder_proof<BlueprintFieldType, placeholder_params>;
            ProofType proof = nil::crypto3::zk::snark::placeholder_prover<BlueprintFieldType, placeholder_params>::process(
                public_preprocessed_data, private_preprocessed_data, table_description, constraint_system, assignment_table,
                fri_params);

            bool verifier_res =
                nil::crypto3::zk::snark::placeholder_verifier<BlueprintFieldType, placeholder_params>::process(
                    public_preprocessed_data, proof, constraint_system, fri_params);

            if (verifier_res) {
                auto filled_placeholder_proof =
                    nil::crypto3::marshalling::types::fill_placeholder_proof<Endianness, ProofType>(proof);
                proof_print<Endianness, ProofType>(proof, proof_file);
                std::cout << "Proof is verified" << std::endl;
                iassignment.close();
            } else {
                std::cout << "Proof is not verified" << std::endl;
                iassignment.close();
            }
        }
    }        // namespace proof_generator
}    // namespace nil

#endif    // PROOF_GENERATOR_ASSIGNER_PROOF_HPP