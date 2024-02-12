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

#ifndef PROOF_GENERATOR_FILE_OPERATIONS_HPP
#define PROOF_GENERATOR_FILE_OPERATIONS_HPP

#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

namespace nil {
    namespace proof_generator {
        inline bool is_valid_path(const std::string& path) {
            if (path.length() >= PATH_MAX) {
                BOOST_LOG_TRIVIAL(error) << path << ": file path is too long. Maximum allowed length is " << PATH_MAX
                                         << " characters.";
                return false;
            }

            if (boost::filesystem::path(path).filename().string().length() >= FILENAME_MAX) {
                BOOST_LOG_TRIVIAL(error) << path << ": file name is too long. Maximum allowed length is "
                                         << FILENAME_MAX << " characters.";
                return false;
            }
            return true;
        }

        inline bool can_read_from_file(const std::string& path) {
            if (!is_valid_path(path)) {
                return false;
            }

            std::ifstream file(path, std::ios::in);
            bool can_read = file.is_open();
            file.close();
            return can_read;
        }

        inline bool can_write_to_file(const std::string& path) {
            if (!is_valid_path(path)) {
                return false;
            }

            if (boost::filesystem::exists(path)) {
                std::ofstream file(path, std::ios::out | std::ios::app);
                bool can_write = file.is_open();
                return can_write;
            } else {
                boost::filesystem::path boost_path = boost::filesystem::absolute(path);
                boost::filesystem::path parent_dir = boost_path.parent_path();
                if (parent_dir.empty()) {
                    BOOST_LOG_TRIVIAL(error) << "Proof parent dir is empty. Seems like you "
                                                "are passing an empty string.";
                    return false;
                }
                if (!boost::filesystem::exists(parent_dir)) {
                    BOOST_LOG_TRIVIAL(error) << boost_path << ": proof parent dir does not exist. Create it first.";
                    return false;
                }
                std::string temp_file_name = (parent_dir / "temp_file_to_test_write_permission").string();
                std::ofstream temp_file(temp_file_name, std::ios::out);
                bool can_write = temp_file.is_open();
                temp_file.close();

                if (can_write) {
                    boost::filesystem::remove(temp_file_name);
                }
                return can_write;
            }
        }

        template<typename StreamType>
        std::optional<StreamType> open_file(
            const std::string& path,
            std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out
        ) {
            StreamType file(path, mode);
            if (!file.is_open()) {
                BOOST_LOG_TRIVIAL(error) << "Unable to open file: " << path;
                return std::nullopt;
            }

            return file;
        }

        auto open_file_r(const std::string& path, std::ios_base::openmode mode = std::ios_base::in) {
            return open_file<std::ifstream>(path, mode);
        }

        auto open_file_w(const std::string& path, std::ios_base::openmode mode = std::ios_base::out) {
            return open_file<std::ofstream>(path, mode);
        }

        std::optional<std::vector<std::uint8_t>> read_file_to_vector(
            const std::string& path,
            std::ios_base::openmode mode = std::ios_base::binary
        ) {
            auto file = open_file_r(path, mode);
            if (!file.has_value()) {
                return std::nullopt;
            }

            std::ifstream& stream = file.value();
            stream.seekg(0, std::ios_base::end);
            const auto fsize = stream.tellg();
            stream.seekg(0, std::ios_base::beg);
            std::vector<std::uint8_t> v(static_cast<size_t>(fsize));

            stream.read(reinterpret_cast<char*>(v.data()), fsize);

            if (stream.fail()) {
                BOOST_LOG_TRIVIAL(error) << "Error occurred during reading file " << path;
                return std::nullopt;
            }

            return v;
        }

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_FILE_OPERATIONS_HPP
