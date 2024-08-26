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

#include "nil/proof-generator/arg_parser.hpp"

#include "nil/proof-generator/arithmetization_params.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

namespace nil {
    namespace proof_generator {
        namespace po = boost::program_options;

        void check_exclusive_options(const po::variables_map& vm, const std::vector<std::string>& opts) {
            std::vector<std::string> found_opts;
            for (const auto& opt : opts) {
                if (vm.count(opt) && !vm[opt].defaulted()) {
                    found_opts.push_back(opt);
                }
            }
            if (found_opts.size() > 1) {
                throw std::logic_error("Conflicting options: " + boost::algorithm::join(found_opts, " and "));
            }
        }

        template<typename T>
        po::typed_value<T>* make_defaulted_option(T& variable) {
            return po::value(&variable)->default_value(variable);
        }

        std::optional<ProverOptions> parse_args(int argc, char* argv[]) {
            po::options_description options("Nil; Proof Generator Options");
            // Declare a group of options that will be
            // allowed only on command line
            po::options_description generic("CLI options");
            // clang-format off
            generic.add_options()
                ("help,h", "Produce help message")
                ("version,v", "Print version string")
                ("config,c", po::value<std::string>(), "Config file path");
            // clang-format on

            ProverOptions prover_options;

            // Declare a group of options that will be
            // allowed both on command line and in
            // config file
            po::options_description config(
                "Configuration",
                /*line_length=*/120,
                /*min_description_length=*/60
            );
            // clang-format off
            auto options_appender = config.add_options()
                ("stage", make_defaulted_option(prover_options.stage),
                 "Stage of the prover to run, one of (all, preprocess, prove, verify). Defaults to 'all'.")
                ("proof,p", make_defaulted_option(prover_options.proof_file_path), "Proof file")
                ("json,j", make_defaulted_option(prover_options.json_file_path), "JSON proof file")
                ("common-data", make_defaulted_option(prover_options.preprocessed_common_data_path), "Preprocessed common data file")
                ("preprocessed-data", make_defaulted_option(prover_options.preprocessed_public_data_path), "Preprocessed public data file")
                ("commitment-state-file", make_defaulted_option(prover_options.commitment_scheme_state_path), "Commitment state data file")
                ("circuit", po::value(&prover_options.circuit_file_path), "Circuit input file")
                ("assignment-table,t", po::value(&prover_options.assignment_table_file_path), "Assignment table input file")
                ("assignment-description-file", po::value(&prover_options.assignment_description_file_path), "Assignment description file")
                ("log-level,l", make_defaulted_option(prover_options.log_level), "Log level (trace, debug, info, warning, error, fatal)")
                ("elliptic-curve-type,e", make_defaulted_option(prover_options.elliptic_curve_type), "Elliptic curve type (pallas)")
                ("hash-type", make_defaulted_option(prover_options.hash_type), "Hash type (keccak, poseidon, sha256)")
                ("lambda-param", make_defaulted_option(prover_options.lambda), "Lambda param (9)")
                ("grind-param", make_defaulted_option(prover_options.grind), "Grind param (69)")
                ("expand-factor,x", make_defaulted_option(prover_options.expand_factor), "Expand factor")
                ("max-quotient-chunks,q", make_defaulted_option(prover_options.max_quotient_chunks), "Maximum quotient polynomial parts amount");

            // clang-format on
            po::options_description cmdline_options("nil; Proof Producer");
            cmdline_options.add(generic).add(config);

            po::variables_map vm;
            try {
                po::store(parse_command_line(argc, argv, cmdline_options), vm);
            } catch (const po::validation_error& e) {
                std::cerr << e.what() << std::endl;
                std::cout << cmdline_options << std::endl;
                throw e;
            }

            if (vm.count("help")) {
                std::cout << cmdline_options << std::endl;
                return std::nullopt;
            }

            if (vm.count("version")) {
#ifdef PROOF_GENERATOR_VERSION
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
                std::cout << TOSTRING(PROOF_GENERATOR_VERSION) << std::endl;
#undef STRINGIFY
#undef TOSTRING
#else
                std::cout << "undefined" << std::endl;
#endif
                return std::nullopt;
            }

            // Parse configuration file. Args from CLI will not be overwritten
            if (vm.count("config")) {
                std::ifstream ifs(vm["config"].as<std::string>().c_str());
                if (ifs) {
                    store(parse_config_file(ifs, config), vm);
                } else {
                    throw std::runtime_error("Cannot open config file: " + vm["config"].as<std::string>());
                }
            }

            // Calling notify(vm) after handling no-op cases prevent parser from alarming
            // about absence of required args
            try {
                notify(vm);
            } catch (const po::required_option& e) {
                std::cerr << e.what() << std::endl;
                std::cout << cmdline_options << std::endl;
                throw e;
            }

            try {
                check_exclusive_options(vm, {"verification-only", "skip-verification"});
            } catch (const std::logic_error& e) {
                std::cerr << e.what() << std::endl;
                std::cout << cmdline_options << std::endl;
                throw e;
            }

            return prover_options;
        }

// Here we have generators of read and write operators for options holding
// types. Don't forget to adjust help message when add new type - name mapping.
// Examples below.
#define GENERATE_WRITE_OPERATOR(TYPE_TO_STRING_LINES, VARIANT_TYPE)             \
    std::ostream& operator<<(std::ostream& strm, const VARIANT_TYPE& variant) { \
        strm << std::visit(                                                     \
            [&strm](auto&& arg) -> std::string {                                \
                using SelectedType = std::decay_t<decltype(arg)>;               \
                TYPE_TO_STRING_LINES                                            \
                strm.setstate(std::ios_base::failbit);                          \
                return "";                                                      \
            },                                                                  \
            variant                                                             \
        );                                                                      \
        return strm;                                                            \
    }
#define TYPE_TO_STRING(TYPE, NAME)                                   \
    if constexpr (std::is_same_v<SelectedType, type_identity<TYPE>>) \
        return NAME;

#define GENERATE_READ_OPERATOR(STRING_TO_TYPE_LINES, VARIANT_TYPE)        \
    std::istream& operator>>(std::istream& strm, VARIANT_TYPE& variant) { \
        std::string str;                                                  \
        strm >> str;                                                      \
        auto l = [&str, &strm]() -> VARIANT_TYPE {                        \
            STRING_TO_TYPE_LINES                                          \
            strm.setstate(std::ios_base::failbit);                        \
            return VARIANT_TYPE();                                        \
        };                                                                \
        variant = l();                                                    \
        return strm;                                                      \
    }
#define STRING_TO_TYPE(TYPE, NAME) \
    if (NAME == str)               \
        return type_identity<TYPE>{};

#define CURVE_TYPES X(nil::crypto3::algebra::curves::pallas, "pallas")
#define X(type, name) TYPE_TO_STRING(type, name)
        GENERATE_WRITE_OPERATOR(CURVE_TYPES, CurvesVariant)
#undef X
#define X(type, name) STRING_TO_TYPE(type, name)
        GENERATE_READ_OPERATOR(CURVE_TYPES, CurvesVariant)
#undef X

#define HASH_TYPES                                                                       \
    X(nil::crypto3::hashes::keccak_1600<256>, "keccak")                                  \
    X(nil::crypto3::hashes::poseidon<nil::crypto3::hashes::detail::mina_poseidon_policy< \
          typename nil::crypto3::algebra::curves::pallas::base_field_type>>,             \
      "poseidon")                                                                        \
    X(nil::crypto3::hashes::sha2<256>, "sha256")
#define X(type, name) TYPE_TO_STRING(type, name)
        GENERATE_WRITE_OPERATOR(HASH_TYPES, HashesVariant)
#undef X
#define X(type, name) STRING_TO_TYPE(type, name)
        GENERATE_READ_OPERATOR(HASH_TYPES, HashesVariant)
#undef X

    } // namespace proof_generator
} // namespace nil
