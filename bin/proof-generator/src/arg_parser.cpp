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

        void print_all_columns_params() {
            std::cout << "Available Policies:\n";
            std::cout << "Index: witness, public input, component constant, component "
                         "selector, lookup constant, "
                         "lookup selector\n";

            for (std::size_t i = 0; i < all_columns_params.size(); ++i) {
                const auto& params = all_columns_params[i];
                std::cout << std::setw(5) << i << ":\t" << params.witness_columns << "," << params.public_input_columns
                          << "," << params.component_constant_columns << "," << params.component_selector_columns << ","
                          << params.lookup_constant_columns << "," << params.lookup_selector_columns << "\n";
            }
        }

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

        std::optional<ProverOptions> parse_args(int argc, char* argv[]) {
            po::options_description options("Nil; Proof Generator Options");
            // Declare a group of options that will be
            // allowed only on command line
            po::options_description generic("CLI options");
            // clang-format off
            generic.add_options()
                ("help,h", "Produce help message")
                ("version,v", "Print version string")
                ("config,c", po::value<std::string>(), "Config file path")
                ("list-columns-params", "Print available columns params");
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
                ("proof,p", po::value(&prover_options.proof_file_path)->default_value(prover_options.proof_file_path), "Output proof file")
                ("common-data", po::value(&prover_options.preprocessed_common_data_path)->default_value(prover_options.preprocessed_common_data_path), "Output preprocessed common data file")
                ("circuit,c", po::value(&prover_options.circuit_file_path)->required(), "Circuit input file")
                ("assignment-table,t", po::value(&prover_options.assignment_table_file_path)->required(), "Assignment table input file")
                ("log-level,l", po::value(&prover_options.log_level)->default_value(prover_options.log_level), "Log level (trace, debug, info, warning, error, fatal)")
                ("elliptic-curve-type,e", po::value(&prover_options.elliptic_curve_type)->default_value(prover_options.elliptic_curve_type), "Elliptic curve type (pallas)")
                ("hash-type", po::value(&prover_options.hash_type)->default_value(prover_options.hash_type), "Hash type (keccak)")
                ("columns-params", po::value(&prover_options.columns)->default_value(prover_options.columns), "Columns params, use --list-columns-params to list")
                ("lambda-param", po::value(&prover_options.lambda)->default_value(prover_options.lambda), "Lambda param (9)")
                ("grind-param", po::value(&prover_options.lambda)->default_value(prover_options.lambda), "Grind param (69)")
                ("expand-factor", po::value(&prover_options.expand_factor)->default_value(prover_options.expand_factor), "Expand factor")
                ("skip-verification", po::bool_switch(&prover_options.skip_verification), "Skip generated proof verifying step")
                ("verification-only", po::bool_switch(&prover_options.verification_only), "Read proof for verification instead of writing to it");
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

            if (vm.count("list-columns-params")) {
                print_all_columns_params();
                return std::nullopt;
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

        // >> and << operators are needed for Boost porgram_options to read values and
        // to print default values to help message: The rest of the file contains them:

        std::ostream& operator<<(std::ostream& strm, const ColumnsParams& columns) {
            auto it = std::find(all_columns_params.cbegin(), all_columns_params.cend(), columns);
            strm << std::distance(all_columns_params.cbegin(), it);
            return strm;
        }

        std::istream& operator>>(std::istream& strm, ColumnsParams& columns) {
            std::string str;
            strm >> str;
            std::size_t pos;
            int idx = std::stoi(str, &pos);
            if (pos < str.size() || idx < 0 || static_cast<std::size_t>(idx) >= all_columns_params.size()) {
                strm.setstate(std::ios_base::failbit);
            } else {
                columns = all_columns_params[idx];
            }
            return strm;
        }

        std::ostream& operator<<(std::ostream& strm, const LambdaParam& lambda) {
            strm << static_cast<size_t>(lambda);
            return strm;
        }

        std::istream& operator>>(std::istream& strm, LambdaParam& lambda) {
            std::string str;
            strm >> str;
            std::size_t pos;
            int val = std::stoi(str, &pos);
            if (pos < str.size() || val < 0) {
                strm.setstate(std::ios_base::failbit);
            } else {
                auto it =
                    std::find(all_lambda_params.cbegin(), all_lambda_params.cend(), static_cast<LambdaParam>(val));
                if (it != all_lambda_params.cend()) {
                    lambda = val;
                } else {
                    strm.setstate(std::ios_base::failbit);
                }
            }
            return strm;
        }

        std::ostream& operator<<(std::ostream& strm, const GrindParam& grind) {
            strm << static_cast<size_t>(grind);
            return strm;
        }

        std::istream& operator>>(std::istream& strm, GrindParam& grind) {
            std::string str;
            strm >> str;
            std::size_t pos;
            int val = std::stoi(str, &pos);
            if (pos < str.size() || val < 0) {
                strm.setstate(std::ios_base::failbit);
            } else {
                auto it = std::find(all_grind_params.cbegin(), all_grind_params.cend(), static_cast<GrindParam>(val));
                if (it != all_grind_params.cend()) {
                    grind = val;
                } else {
                    strm.setstate(std::ios_base::failbit);
                }
            }
            return strm;
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

#define HASH_TYPES X(nil::crypto3::hashes::keccak_1600<256>, "keccak")
#define X(type, name) TYPE_TO_STRING(type, name)
        GENERATE_WRITE_OPERATOR(HASH_TYPES, HashesVariant)
#undef X
#define X(type, name) STRING_TO_TYPE(type, name)
        GENERATE_READ_OPERATOR(HASH_TYPES, HashesVariant)
#undef X

    } // namespace proof_generator
} // namespace nil
