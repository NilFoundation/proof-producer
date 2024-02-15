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
        std::optional<StreamType> open_file(const std::string& path, std::ios_base::openmode mode) {
            StreamType file(path, mode);
            if (!file.is_open()) {
                BOOST_LOG_TRIVIAL(error) << "Unable to open file: " << path;
                return std::nullopt;
            }

            return file;
        }

        std::optional<std::vector<std::uint8_t>> read_file_to_vector(const std::string& path) {

            auto file = open_file<std::ifstream>(path, std::ios_base::in | std::ios::binary | std::ios::ate);
            if (!file.has_value()) {
                return std::nullopt;
            }

            std::ifstream& stream = file.value();
            std::streamsize fsize = stream.tellg();
            stream.seekg(0, std::ios::beg);
            std::vector<std::uint8_t> v(static_cast<size_t>(fsize));

            stream.read(reinterpret_cast<char*>(v.data()), fsize);

            if (stream.fail()) {
                BOOST_LOG_TRIVIAL(error) << "Error occurred during reading file " << path;
                return std::nullopt;
            }

            return v;
        }

        bool write_vector_to_file(const std::vector<std::uint8_t>& vector, const std::string& path) {

            auto file = open_file<std::ofstream>(path, std::ios_base::out | std::ios_base::binary);
            if (!file.has_value()) {
                return false;
            }

            std::ofstream& stream = file.value();
            stream.write(reinterpret_cast<const char*>(vector.data()), vector.size());

            if (stream.fail()) {
                BOOST_LOG_TRIVIAL(error) << "Error occured during writing file " << path;
                return false;
            }

            return true;
        }

        // HEX data format is not efficient, we will remove it later
        std::optional<std::vector<std::uint8_t>> read_hex_file_to_vector(const std::string& path) {
            auto file = open_file<std::ifstream>(path, std::ios_base::in);
            if (!file.has_value()) {
                return std::nullopt;
            }

            std::ifstream& stream = file.value();
            std::vector<uint8_t> result;
            std::string line;
            while (std::getline(stream, line)) {
                if (line.rfind("0x", 0) == 0 && line.length() >= 3) {
                    for (size_t i = 2; i < line.length(); i += 2) {
                        std::string hex_string = line.substr(i, 2);
                        uint8_t byte = static_cast<uint8_t>(std::stoul(hex_string, nullptr, 16));
                        result.push_back(byte);
                    }
                } else {
                    BOOST_LOG_TRIVIAL(error) << "File contains non-hex string";
                    return std::nullopt;
                }
            }

            return result;
        }

        bool write_vector_to_hex_file(const std::vector<std::uint8_t>& vector, const std::string& path) {
            auto file = open_file<std::ofstream>(path, std::ios_base::out);
            if (!file.has_value()) {
                return false;
            }

            std::ofstream& stream = file.value();

            stream << "0x" << std::hex;
            for (auto it = vector.cbegin(); it != vector.cend(); ++it) {
                stream << std::setfill('0') << std::setw(2) << std::right << int(*it);
            }
            stream << std::dec;

            if (stream.fail()) {
                BOOST_LOG_TRIVIAL(error) << "Error occurred during writing to file " << path;
                return false;
            }

            return true;
        }

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_FILE_OPERATIONS_HPP
