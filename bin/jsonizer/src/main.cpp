//---------------------------------------------------------------------------//
// Copyright (c) 2024 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2024 Elena Tatuzova <e.tatuzova@nil.foundation>
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

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <iostream>

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#endif

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/json/src.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

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

#include <nil/proof-producer/recursive_json_generator.hpp>

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

template <typename BlueprintFieldType, typename ArithmetizationParams>
int instanciated_main(boost::filesystem::path proof_file_path,
                      boost::filesystem::path assignment_table_file_path,
                      boost::filesystem::path circuit_file_path,
                      std::size_t used_public_input_rows,
                      std::size_t used_shared_rows) {

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

    TableDescriptionType table_description;
    AssignmentTableType assignment_table;

    {
        std::ifstream iassignment;
        iassignment.open(assignment_table_file_path, std::ios_base::binary | std::ios_base::in);
        if (!iassignment) {
            BOOST_LOG_TRIVIAL(error) << "Cannot open " << assignment_table_file_path;
            return false;
        }
        std::vector<std::uint8_t> v;
        iassignment.seekg(0, std::ios_base::end);
        const auto fsize = iassignment.tellg();
        v.resize(fsize);
        iassignment.seekg(0, std::ios_base::beg);
        iassignment.read(reinterpret_cast<char*>(v.data()), fsize);
        if (!iassignment) {
            BOOST_LOG_TRIVIAL(error) << "Cannot parse input file " << assignment_table_file_path;
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

    std::array<std::size_t, ArithmetizationParams::public_input_columns> public_input_sizes;
    for(std::size_t i = 0; i < ArithmetizationParams::public_input_columns; i++){
        public_input_sizes[i] = used_public_input_rows;
    }
    if(ArithmetizationParams::public_input_columns > 1 && used_shared_rows > 0){
        public_input_sizes[ArithmetizationParams::public_input_columns - 1] = used_shared_rows;
    }

    using ProofType = nil::crypto3::zk::snark::placeholder_proof<BlueprintFieldType, placeholder_params>;
        using proof_marshalling_type =
        nil::crypto3::marshalling::types::placeholder_proof<TTypeBase, ProofType>;

    ProofType proof;
    BOOST_LOG_TRIVIAL(info) << "Proof Type = " << typeid(ProofType).name() << std::endl;
    {
        std::ifstream iproof;
        iproof.open(proof_file_path, std::ios_base::binary | std::ios_base::in);
        if (!iproof) {
            BOOST_LOG_TRIVIAL(error) << "Cannot open " << proof_file_path;
            return false;
        }

        std::vector<std::uint8_t> v;
        if (!read_buffer_from_file(iproof, v)) {
            BOOST_LOG_TRIVIAL(error) << "Cannot parse input file " << proof_file_path << std::endl;
            return false;
        }

        proof_marshalling_type marshalled_proof_data;
        auto read_iter = v.begin();
        auto status = marshalled_proof_data.read(read_iter, v.size());
        proof = nil::crypto3::marshalling::types::make_placeholder_proof<Endianness, ProofType>(
            marshalled_proof_data
        );
    }

    proof_file_path.replace_extension(".json");
    std::ofstream json_proof_file_stream;
    json_proof_file_stream.open(proof_file_path);
    json_proof_file_stream << nil::proof_producer::recursive_json_generator<
        placeholder_params,
        nil::crypto3::zk::snark::placeholder_proof<BlueprintFieldType, placeholder_params>
    >::generate_proof_json(
        proof, assignment_table.public_inputs(), public_input_sizes
    );
    json_proof_file_stream.close();
    BOOST_LOG_TRIVIAL(info) << "JSON written" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {

    boost::program_options::options_description options_desc("zkLLVM proof2json recursive verifier input creation tool options");

    // clang-format off
    options_desc.add_options()("help,h", "Display help message")
            ("version,v", "Display version")
            ("proof,b", boost::program_options::value<std::string>(), "Bytecode input file with the proof")
            ("assignment-table,t", boost::program_options::value<std::string>(), "Assignment table file - required for public preprocessed data generation.")
            ("circuit,c", boost::program_options::value<std::string>(), "Circuit file with the constraint system - required for public preprocessed data generation.")
            ("used-public-input-rows,p", boost::program_options::value<std::size_t>(), "Public input columns expected size")
            ("used-shared-rows,s", boost::program_options::value<std::size_t>(), "Shared column expected size")
            ("log-level,l", boost::program_options::value<std::string>(), "Log level (trace, debug, info, warning, error, fatal)")
            ("elliptic-curve-type,e", boost::program_options::value<std::string>(), "Native elliptic curve type (pallas, vesta, ed25519, bls12381)")
            ;
    // clang-format on


    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options_desc).run(),
                                    vm);
        boost::program_options::notify(vm);
    } catch (const boost::program_options::unknown_option &e) {
        BOOST_LOG_TRIVIAL(error) << "Invalid command line argument: " << e.what();
        std::cout << options_desc << std::endl;
        return 1;
    }


    if (vm.count("help")) {
        std::cout << options_desc << std::endl;
        return 0;
    }

    if (vm.count("version")) {
#ifdef PROOF2JSON_VERSION
#define xstr(s) str(s)
#define str(s) #s
        std::cout << xstr(PROOF2JSON_VERSION) << std::endl;
#else
        std::cout << "undefined" << std::endl;
#endif
        return 0;
    }

    boost::filesystem::path proof_file_path;
    boost::filesystem::path assignment_table_file_path;
    boost::filesystem::path circuit_file_path;
    std::size_t used_public_input_rows;
    std::size_t used_shared_rows;
    std::string log_level;

    // We use Boost log trivial severity levels, these are string representations of their names
    std::map<std::string, boost::log::trivial::severity_level> log_options{
        {"trace", boost::log::trivial::trace},
        {"debug", boost::log::trivial::debug},
        {"info", boost::log::trivial::info},
        {"warning", boost::log::trivial::warning},
        {"error", boost::log::trivial::error},
        {"fatal", boost::log::trivial::fatal}
    };

    if (vm.count("log-level")) {
        log_level = vm["log-level"].as<std::string>();
    } else {
        log_level = "info";
    }

    if (log_options.find(log_level) == log_options.end()) {
        BOOST_LOG_TRIVIAL(error) << "Invalid command line argument -l (log level): " << log_level << std::endl;
        std::cout << options_desc << std::endl;
        return 1;
    }

    boost::log::core::get()->set_filter(boost::log::trivial::severity >= log_options[log_level]);

    if (vm.count("proof")) {
        proof_file_path = vm["proof"].as<std::string>();
    } else {
        proof_file_path = boost::filesystem::current_path() / "proof.bin";
        BOOST_LOG_TRIVIAL(debug) << "Proof file path not specified, using default: " << proof_file_path;
    }

    if (vm.count("assignment-table")) {
        assignment_table_file_path = boost::filesystem::path(vm["assignment-table"].as<std::string>());
        BOOST_LOG_TRIVIAL(debug) << "Assignment table file path: " << assignment_table_file_path;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Invalid command line argument - assignment table file name is not specified";
        std::cout << options_desc << std::endl;
        return 1;
    }

    if (vm.count("circuit")) {
        circuit_file_path = boost::filesystem::path(vm["circuit"].as<std::string>());
        BOOST_LOG_TRIVIAL(debug) << "Circuit file path: " << circuit_file_path;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Invalid command line argument - circuit file name is not specified";
        std::cout << options_desc << std::endl;
        return 1;
    }

    if (vm.count("used-public-input-rows")) {
        used_public_input_rows = vm["used-public-input-rows"].as<std::size_t>();
    } else {
        used_public_input_rows = 50;
    }

    if (vm.count("used-shared-rows")) {
        used_shared_rows = vm["used-shared-rows"].as<std::size_t>();
    } else {
        used_shared_rows = 0;
    }

    std::string elliptic_curve;

    if (vm.count("elliptic-curve-type")) {
        elliptic_curve = vm["elliptic-curve-type"].as<std::string>();
    } else {
        std::cerr << "Invalid command line argument - elliptic curve type is not specified" << std::endl;
        std::cout << options_desc << std::endl;
        return 1;
    }

    std::map<std::string, int> curve_options{
        {"pallas", 0},
        {"vesta", 1},
        {"ed25519", 2},
        {"bls12381", 3},
    };

    if (curve_options.find(elliptic_curve) == curve_options.end()) {
        std::cerr << "Invalid command line argument -e (Native elliptic curve type): " << elliptic_curve << std::endl;
        std::cout << options_desc << std::endl;
        return 1;
    }


    constexpr std::size_t ComponentConstantColumns = 5;
    constexpr std::size_t LookupConstantColumns = 30;
    constexpr std::size_t ComponentSelectorColumns = 30;
    constexpr std::size_t LookupSelectorConstantColumns = 6;

    constexpr std::size_t WitnessColumns = 15;
    constexpr std::size_t PublicInputColumns = 1;
    constexpr std::size_t ConstantColumns = ComponentConstantColumns + LookupConstantColumns;
    constexpr std::size_t SelectorColumns = ComponentSelectorColumns + LookupSelectorConstantColumns;

    using ArithmetizationParams =
                nil::crypto3::zk::snark::plonk_arithmetization_params<WitnessColumns, PublicInputColumns, ConstantColumns,
                                                                    SelectorColumns>;

    switch (curve_options[elliptic_curve]) {
        case 0: {
            using curve_type = nil::crypto3::algebra::curves::pallas;
            using BlueprintFieldType = typename curve_type::base_field_type;
            return instanciated_main<BlueprintFieldType, ArithmetizationParams>(proof_file_path, assignment_table_file_path, circuit_file_path, used_public_input_rows, used_shared_rows);
        }
        case 1: {
            BOOST_LOG_TRIVIAL(error) << "vesta curve based circuits are not supported yet";
            return 1;
        }
        case 2: {
            BOOST_LOG_TRIVIAL(error) << "ed25519 curve based circuits are not supported yet";
            return 1;
        }
        case 3: {
            using curve_type = nil::crypto3::algebra::curves::bls12<381>;
            using BlueprintFieldType = typename curve_type::base_field_type;
            BOOST_LOG_TRIVIAL(error) << "bls12-381 curve based circuits proving is temporarily disabled";
        }
    };

}