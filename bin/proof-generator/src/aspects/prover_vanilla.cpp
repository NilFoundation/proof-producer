//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#include <nil/proof-generator/aspects/prover_vanilla.hpp>

#include <boost/exception/get_error_info.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>

namespace std {
    template<typename CharT, typename TraitsT>
    std::basic_ostream<CharT, TraitsT> &operator<<(std::basic_ostream<CharT, TraitsT> &out,
                                                   const std::vector<std::string> &xs) {
        return out << std::accumulate(
                   std::next(xs.begin()), xs.end(), xs[0],
                   [](std::string a, const std::string &b) -> std::string { return std::move(a) + ';' + b; });
    }
}    // namespace std

namespace nil {
    namespace proof_generator {
        namespace aspects {
            prover_vanilla::prover_vanilla(boost::shared_ptr<path> aspct) : path_aspect(std::move(aspct)) {
            }

            void prover_vanilla::set_options(cli_options_type &cli) const {
                boost::program_options::options_description options("NIL Proof Generator");
                // clang-format off
                options.add_options()
                ("version,v", "Display version")
                ("proof", boost::program_options::value<std::string>(),"Output proof file")
                ("circuit,c", boost::program_options::value<std::string>(), "Circuit input file")
                ("assignment-table,t", boost::program_options::value<std::string>(), "Assignment table input file");
                // clang-format on
                cli.add(options);
            }

            void prover_vanilla::set_options(cfg_options_type &cfg) const {
                boost::program_options::options_description options("NIL Proof Generator");
                // clang-format off
                options.add_options()
                ("version,v", "Display version")
                ("proof", boost::program_options::value<std::string>(),"Output proof file")
                ("circuit,c", boost::program_options::value<std::string>(), "Circuit input file")
                ("assignment-table,t", boost::program_options::value<std::string>(), "Assignment table input file");
                // clang-format on
                cfg.add(options);
            }

            void prover_vanilla::initialize(configuration_type &vm) {
                if (vm.count("circuit")) {
                    if (vm["circuit"].as<std::string>().size() < PATH_MAX ||
                        vm["circuit"].as<std::string>().size() < FILENAME_MAX) {
                        if (boost::filesystem::exists(vm["circuit"].as<std::string>())) {
                            circuit_file_path = vm["circuit"].as<std::string>();
                        }
                    }
                }
                if (vm.count("assignment-table")) {
                    if (vm["assignment-table"].as<std::string>().size() < PATH_MAX ||
                        vm["assignment-table"].as<std::string>().size() < FILENAME_MAX) {
                        if (boost::filesystem::exists(vm["assignment-table"].as<std::string>())) {
                            assignment_table_file_path = vm["assignment-table"].as<std::string>();
                        }
                    }
                }
                if (vm.count("proof")) {
                    proof_file_path = vm["proof"].as<std::string>();
                }
            }

            boost::filesystem::path prover_vanilla::default_config_path() const {
                return path_aspect->config_path() / "config.ini";
            }

            boost::filesystem::path  prover_vanilla::input_circuit_file_path() const {
                return circuit_file_path;
            }

            boost::filesystem::path prover_vanilla::input_assignment_file_path() const {
                return assignment_table_file_path;
            }

            boost::filesystem::path prover_vanilla::output_proof_file_path() const {
                return proof_file_path;
            }
        }    // namespace aspects
    }        // namespace proof_generator
}    // namespace nil
