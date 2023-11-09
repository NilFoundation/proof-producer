//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021-2022 Aleksei Moskvin <alalmoskvin@gmail.com>
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
#include <nil/crypto3/hash/blake2b.hpp>

#include <nil/crypto3/container/merkle/tree.hpp>
#include <nil/crypto3/container/merkle/proof.hpp>

#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/metrics_api.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/thread.hh>

#include <nil/actor/container/merkle/tree.hpp>

using namespace nil::crypto3;
using namespace nil::crypto3::containers;

int main(int argc, char **argv) {
    nil::actor::app_template app;
    app.run(argc, argv, [] {
        return nil::actor::async([] {
            std::vector<std::array<char, 1>> data_on_leafs = {{'0'}, {'1'}, {'2'}, {'3'}, {'4'},
                                                              {'5'}, {'6'}, {'7'}, {'8'}};
            std::array<char, 1> element_not_in_tree = {'9'};
            nil::crypto3::containers::merkle_tree<hashes::blake2b<224>, 3> tree =
                nil::actor::containers::make_merkle_tree<hashes::blake2b<224>, 3>(data_on_leafs.begin(),
                                                                                  data_on_leafs.end()).get();

            nil::crypto3::containers::merkle_proof<hashes::blake2b<224>, 3> proof_leaf_3(tree, 3);
            nil::crypto3::containers::merkle_proof<hashes::blake2b<224>, 3> proof_leaf_0(tree, 0);

            std::vector<std::array<char, 1>> data_to_check = {
                {data_on_leafs[2]}, {data_on_leafs[0]}, {data_on_leafs[3]}, element_not_in_tree};
            for (size_t i = 0; i < data_to_check.size(); ++i) {
                std::cout << "Is leaf " << data_to_check[i][0] << " was in tree in position 0: ";
                std::cout << std::boolalpha << proof_leaf_0.validate(data_to_check[i]) << std::endl;
                std::cout << "Is leaf " << data_to_check[i][0] << " was in tree in position 3: ";
                std::cout << std::boolalpha << proof_leaf_3.validate(data_to_check[i]) << std::endl;
            }

            std::array<char, 7> left = {'\x6d', '\x65', '\x73', '\x73', '\x61', '\x67', '\x65'};
            std::array<char, 7> right = {'\x20', '\x64', '\x69', '\x67', '\x65', '\x73', '\x74'};
            std::vector<std::array<char, 7>> simple_binary_tree_data = {left, right};
            nil::crypto3::containers::merkle_tree<hashes::blake2b<224>, 2> simple_binary_tree =
                nil::actor::containers::make_merkle_tree<hashes::blake2b<224>, 2>(simple_binary_tree_data.begin(),
                                                                                  simple_binary_tree_data.end()).get();
            nil::crypto3::containers::merkle_proof<hashes::blake2b<224>, 2> simple_binary_proof_leaf_1(
                simple_binary_tree, 1);
            //    std::cout << "Tree simple binary structure:" << std::endl;
            //    std::cout << simple_binary_tree << std::endl;
            std::cout << "Is leaf " << data_on_leafs[1][0] << " was in tree in position 1: ";
            std::cout << std::boolalpha << simple_binary_proof_leaf_1.validate(data_on_leafs[1]) << std::endl;
            std::cout << "Is leaf left was in tree in position 1: ";
            std::cout << std::boolalpha << simple_binary_proof_leaf_1.validate(left) << std::endl;
            std::cout << "Is leaf right was in tree in position 1: ";
            std::cout << std::boolalpha << simple_binary_proof_leaf_1.validate(right) << std::endl;

            return 0;
        });
    });
}