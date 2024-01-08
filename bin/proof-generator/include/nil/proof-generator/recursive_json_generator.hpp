//---------------------------------------------------------------------------//
// Copyright (c) 2023 Elena Tatuzova <e.tatuzova@nil.foundation>
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
// @file Declaration of interfaces for PLONK unified addition component.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_RECURSIVE_JSON_GENERATOR_HPP
#define CRYPTO3_RECURSIVE_JSON_GENERATOR_HPP

#include <sstream>
#include <map>

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include<nil/crypto3/hash/keccak.hpp>
#include<nil/crypto3/hash/sha2.hpp>

#include<nil/crypto3/hash/sha2.hpp>
#include <nil/crypto3/algebra/random_element.hpp>

#include <nil/crypto3/zk/math/expression.hpp>
#include <nil/crypto3/zk/math/expression_visitors.hpp>
#include <nil/crypto3/zk/math/expression_evaluator.hpp>

namespace nil {
    namespace proof_generator {
         template<typename T> std::string to_string(T val) {
            std::stringstream strstr;
            strstr << val;
            return strstr.str();
        }

        template<typename T> std::string to_hex_string(T val) {
            std::stringstream strstr;
            strstr << std::hex << val << std::dec;
            return strstr.str();
        }

        template<typename PlaceholderParams, typename ProofType, typename CommonDataType>
        struct recursive_json_generator{
            using field_type = typename PlaceholderParams::field_type;
            using arithmetization_params = typename PlaceholderParams::arithmetization_params;
            using proof_type = ProofType;
            using common_data_type = CommonDataType;
            using verification_key_type = typename common_data_type::verification_key_type;
            using commitment_scheme_type = typename PlaceholderParams::commitment_scheme_type;
            using constraint_system_type = typename PlaceholderParams::constraint_system_type;
            using columns_rotations_type = std::array<std::set<int>, PlaceholderParams::total_columns>;
            using variable_type = typename constraint_system_type::variable_type;
            using variable_indices_type = std::map<variable_type, std::size_t>;
            using degree_visitor_type = typename constraint_system_type::degree_visitor_type;
            using expression_type = typename constraint_system_type::expression_type;
            using term_type = typename constraint_system_type::term_type;
            using binary_operation_type = typename constraint_system_type::binary_operation_type;
            using pow_operation_type = typename constraint_system_type::pow_operation_type;
            using assignment_table_type = typename PlaceholderParams::assignment_table_type;


            static std::string generate_field_array2_from_64_hex_string(std::string str){
                BOOST_ASSERT_MSG(str.size() == 64, "input string must be 64 hex characters long");
                std::string first_half = str.substr(0, 32);
                std::string second_half = str.substr(32, 32);
                return  "{\"vector\": [{\"field\": \"0x" + first_half + "\"},{\"field\": \"0x" + second_half + "\"}]}";
            }

            template<typename HashType>
            static inline std::string generate_hash(typename HashType::digest_type hashed_data){
                if constexpr(std::is_same<HashType, nil::crypto3::hashes::sha2<256>>::value){
                    std::stringstream out;
                    out << hashed_data;
                    return generate_field_array2_from_64_hex_string(out.str());
                } else if constexpr(std::is_same<HashType, nil::crypto3::hashes::keccak_1600<256>>::value){
                    return "{\"string\": \"keccak\"}";
                } else {
                    std::stringstream out;
                    out << "{\"field\": \"" <<  hashed_data <<  "\"}";
                    return out.str();
                }
                BOOST_ASSERT_MSG(false, "unsupported merkle hash type");
                return "unsupported merkle hash type";
            }

            template<typename CommitmentSchemeType>
            static inline std::string generate_commitment(typename CommitmentSchemeType::commitment_type commitment) {
                return generate_hash<typename CommitmentSchemeType::lpc::merkle_hash_type>(commitment);
            }

            template<typename CommitmentSchemeType>
            static inline std::string generate_eval_proof(typename CommitmentSchemeType::proof_type eval_proof) {
                if( CommitmentSchemeType::lpc::use_grinding ){
                    BOOST_ASSERT_MSG(false, "grinding is not supported");
                    std::cout << "Grinding is not supported" << std::endl;
                    return "Grinding is not supported";
                }

                std::stringstream out;
                out << "\t\t{\"array\":[" << std::endl;
                auto batch_info = eval_proof.z.get_batch_info();
                std::size_t sum = 0;
                std::size_t poly_num = 0;
                for(const auto& [k, v]: batch_info){
                    for(std::size_t i = 0; i < v; i++){
                        poly_num++;
                        BOOST_ASSERT(eval_proof.z.get_poly_points_number(k, i) != 0);
                        for(std::size_t j = 0; j < eval_proof.z.get_poly_points_number(k, i); j++){
                            if( sum != 0 ) out << "," << std::endl;
                            out << "\t\t\t{\"field\":\"" << eval_proof.z.get(k, i, j) << "\"}";
                            sum++;
                        }
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;
                out << "\t\t{\"array\": [" << std::endl;
                for( std::size_t i = 0; i < eval_proof.fri_proof.fri_roots.size(); i++){
                    if(i != 0) out << "," << std::endl;
                    out << "\t\t\t" << generate_commitment<CommitmentSchemeType>(
                        eval_proof.fri_proof.fri_roots[i]
                    );
                }
                out << std::endl << "\t\t]}," << std::endl;
                out << "\t\t{\"array\": [" << std::endl;
                std::size_t cur = 0;
                for( std::size_t i = 0; i < eval_proof.fri_proof.query_proofs.size(); i++){
                    for( const auto &[j, initial_proof]: eval_proof.fri_proof.query_proofs[i].initial_proof){
                        for( std::size_t k = 0; k < initial_proof.values.size(); k++){
                            if(cur != 0) out << "," << std::endl;
                            BOOST_ASSERT_MSG(initial_proof.values[k].size() == 1, "Unsupported step_list[0] value");
                            out << "\t\t\t{\"field\":\"" << initial_proof.values[k][0][0] << "\"}," << std::endl;
                            out << "\t\t\t{\"field\":\"" << initial_proof.values[k][0][1] << "\"}";
                            cur++;
                            cur++;
                        }
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;
                out << "\t\t{\"array\": [" << std::endl;
                cur = 0;
                for( std::size_t i = 0; i < eval_proof.fri_proof.query_proofs.size(); i++){
                    for( std::size_t j = 0; j < eval_proof.fri_proof.query_proofs[i].round_proofs.size(); j++){
                        const auto &round_proof = eval_proof.fri_proof.query_proofs[i].round_proofs[j];
                        if(cur != 0) out << "," << std::endl;
                        BOOST_ASSERT_MSG(round_proof.y.size() == 1, "Unsupported step_lis value");
                        out << "\t\t\t{\"field\":\"" << round_proof.y[0][0] << "\"}," << std::endl;
                        out << "\t\t\t{\"field\":\"" << round_proof.y[0][1] << "\"}";
                        cur++;
                        cur++;
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;

                out << "\t\t{\"array\": [" << std::endl;
                cur = 0;
                for( std::size_t i = 0; i < eval_proof.fri_proof.query_proofs.size(); i++){
                    for( const auto &[j, initial_proof]: eval_proof.fri_proof.query_proofs[i].initial_proof){
                        for( std::size_t k = 0; k < initial_proof.p.path().size(); k++){
                            if(cur != 0) out << "," << std::endl;
                            out << "\t\t\t{\"int\":" << initial_proof.p.path()[k][0].position() << "}";
                            cur ++;
                        }
                        break;
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;

                out << "\t\t{\"array\": [" << std::endl;
                cur = 0;
                for( std::size_t i = 0; i < eval_proof.fri_proof.query_proofs.size(); i++){
                    for( const auto &[j, initial_proof]: eval_proof.fri_proof.query_proofs[i].initial_proof){
                        for( std::size_t k = 0; k < initial_proof.p.path().size(); k++){
                            if(cur != 0) out << "," << std::endl;
                            out << "\t\t\t" << generate_hash<typename CommitmentSchemeType::lpc::merkle_hash_type>(
                                initial_proof.p.path()[k][0].hash()
                            );
                            cur ++;
                        }
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;

                out << "\t\t{\"array\": [" << std::endl;
                cur = 0;
                for( std::size_t i = 0; i < eval_proof.fri_proof.query_proofs.size(); i++){
                    for( std::size_t j = 0; j < eval_proof.fri_proof.query_proofs[i].round_proofs.size(); j++){
                        const auto& p = eval_proof.fri_proof.query_proofs[i].round_proofs[j].p;
                        for( std::size_t k = 0; k < p.path().size(); k++){
                            if(cur != 0) out << "," << std::endl;
                            out << "\t\t\t{\"int\": " << p.path()[k][0].position() << "}";
                            cur++;
                        }
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;

                out << "\t\t{\"array\": [" << std::endl;
                cur = 0;
                for( std::size_t i = 0; i < eval_proof.fri_proof.query_proofs.size(); i++){
                    for( std::size_t j = 0; j < eval_proof.fri_proof.query_proofs[i].round_proofs.size(); j++){
                        const auto& p = eval_proof.fri_proof.query_proofs[i].round_proofs[j].p;
                        for( std::size_t k = 0; k < p.path().size(); k++){
                            if(cur != 0) out << "," << std::endl;
                            out << "\t\t\t" << generate_hash<typename CommitmentSchemeType::lpc::merkle_hash_type>(
                                p.path()[k][0].hash()
                            );
                            cur++;
                        }
                    }
                }
                out << std::endl << "\t\t]}," << std::endl;

                cur = 0;
                out << "\t\t{\"array\": [" << std::endl;
                for( std::size_t i = 0; i < eval_proof.fri_proof.final_polynomial.size(); i++){
                    if(cur != 0) out << "," << std::endl;
                    out << "\t\t\t{\"field\": \"" << eval_proof.fri_proof.final_polynomial[i] << "\"}";
                    cur++;
                }
                out << std::endl << "\t\t]}";

                return out.str();
            }

            static inline std::string generate_input(
                const verification_key_type &vk,
                const typename assignment_table_type::public_input_container_type &public_inputs,
                const proof_type &proof,
                const std::array<std::size_t, arithmetization_params::public_input_columns> public_input_sizes
            ){
                std::stringstream out;
                out << "[" << std::endl;

                out << "\t{\"array\":[" << std::endl;
                std::size_t cur = 0;
                for(std::size_t i = 0; i < arithmetization_params::public_input_columns; i++){
                    for(std::size_t j = 0; j < public_input_sizes[i]; j++){
                        if(cur != 0) out << "," << std::endl;
                        out << "\t\t{\"field\": \"" << public_inputs[i][j] << "\"}";
                        cur++;
                    }
                }
                out << std::endl << "\t]}," << std::endl;

                out << "\t{\"array\":[" << std::endl;
                out << "\t\t" << generate_hash<typename PlaceholderParams::transcript_hash_type>(
                    vk.constraint_system_hash
                ) << "," << std::endl;
                out << "\t\t" << generate_hash<typename PlaceholderParams::transcript_hash_type>(
                    vk.fixed_values_commitment
                ) << std::endl;
                out << "\t]}," << std::endl;

                out << "\t{\"struct\":[" << std::endl;
                out << "\t\t{\"array\":[" << std::endl;
                out << "\t\t\t" << generate_commitment<typename PlaceholderParams::commitment_scheme_type>(proof.commitments.at(1))//(nil::crypto3::zk::snark::VARIABLE_VALUES_BATCH)
                    << "," << std::endl;
                out << "\t\t\t" << generate_commitment<typename PlaceholderParams::commitment_scheme_type>(
                    proof.commitments.at(2))//(nil::crypto3::zk::snark::PERMUTATION_BATCH)
                    << "," << std::endl;
                out << "\t\t\t" << generate_commitment<typename PlaceholderParams::commitment_scheme_type>(
                    proof.commitments.at(3)) // (nil::crypto3::zk::snark::QUOTIENT_BATCH)
                ;

                if( proof.commitments.find(4) != proof.commitments.end() ){ /*nil::crypto3::zk::snark::LOOKUP_BATCH*/
                    out << "," << std::endl << "\t\t\t" << generate_commitment<typename PlaceholderParams::commitment_scheme_type>(
                        proof.commitments.at(4) //nil::crypto3::zk::snark::LOOKUP_BATCH)
                    );
                }
                out << std::endl;

                out << "\t\t]}," << std::endl;
                out << "\t\t{\"field\": \"" << proof.eval_proof.challenge << "\"}," << std::endl;
                out << generate_eval_proof<typename PlaceholderParams::commitment_scheme_type>(
                    proof.eval_proof.eval_proof
                ) << std::endl;
                out << "\t]}" << std::endl;

                out << "]" << std::endl;
                return out.str();
            }
        };
    }
}

#endif   // CRYPTO3_RECURSIVE_JSON_GENERATOR_HPP