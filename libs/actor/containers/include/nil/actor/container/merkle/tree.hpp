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

#ifndef ACTOR_MERKLE_TREE_HPP
#define ACTOR_MERKLE_TREE_HPP

#include <vector>
#include <cmath>

#include <boost/config.hpp>

#include <nil/crypto3/detail/static_digest.hpp>
#include <nil/crypto3/detail/type_traits.hpp>

#include <nil/crypto3/hash/algorithm/hash.hpp>

#include <nil/actor/core/future.hh>
#include <nil/crypto3/container/merkle/node.hpp>
#include <nil/crypto3/container/merkle/tree.hpp>

#include <nil/actor/core/when_all.hh>
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/distributed.hh>
#include <nil/actor/core/shared_ptr.hh>

namespace nil {
    namespace actor {
        namespace containers {
            namespace detail {

                template<typename T, std::size_t Arity, typename LeafIterator>
                nil::crypto3::containers::detail::merkle_tree_impl<T, Arity> make_merkle_tree(LeafIterator first,
                                                                                              LeafIterator last) {
                    typedef T node_type;
                    typedef typename node_type::hash_type hash_type;
                    typedef typename node_type::value_type value_type;

                    std::size_t number_leaves = std::distance(first, last);
                    nil::crypto3::containers::detail::merkle_tree_impl<T, Arity> ret(number_leaves);
                    std::size_t core_count = nil::actor::smp::count;
                    ret.resize(ret.complete_size());

                    auto ret_p = make_foreign(&ret);
                    std::size_t parallels = std::min(core_count, number_leaves);
                    std::vector<nil::actor::future<>> fut;
                    for (auto i = 0; i < parallels; ++i) {
                        auto begin = i * (number_leaves / parallels);
                        auto end = (i + 1) * (number_leaves / parallels);
                        if (i == parallels - 1) {
                            end = number_leaves;
                        }
                        fut.emplace_back(nil::actor::smp::submit_to(i, [begin, end, first, p = ret_p.get()]() {
                            auto itr = first;
                            for (auto i = begin; i < end; ++i) {
                                (*p)[i] = (value_type)crypto3::hash<hash_type>(*itr++);
                            }
                            return nil::actor::make_ready_future<>();
                        }));
                        first += number_leaves / parallels;
                    }
                    nil::actor::when_all(fut.begin(), fut.end()).get();

                    std::size_t row_size = ret.leaves() / Arity;
                    typename nil::crypto3::containers::detail::merkle_tree_impl<T, Arity>::iterator it = ret.begin();
                    std::size_t current_pos = number_leaves;
                    for (size_t row_number = 1; row_number < ret.row_count(); ++row_number, row_size /= Arity) {
                        fut.clear();
                        parallels = std::min(core_count, row_size);
                        std::size_t node_per_shard = row_size / parallels;

                        for (std::size_t c = 0; c < parallels; ++c) {
                            auto begin_row = node_per_shard * c;
                            auto end_row = node_per_shard * (c + 1);
                            if (c == parallels - 1) {
                                end_row = row_size;
                            }
                            auto it_c = it + node_per_shard * c * Arity;

                            fut.push_back(
                                nil::actor::smp::submit_to(c, [begin_row, end_row, it_c, current_pos, p = ret_p.get()] {
                                    auto index_pos = current_pos;
                                    for (size_t i = 0; i < end_row - begin_row; ++i, ++index_pos) {
                                        (*p)[index_pos] = nil::crypto3::containers::detail::generate_hash<hash_type>(
                                            it_c + i * Arity, it_c + (i + 1) * Arity);
                                    }
                                    return nil::actor::make_ready_future<>();
                                }));
                            current_pos += end_row - begin_row;
                        }

                        it += Arity * row_size;

                        nil::actor::when_all(fut.begin(), fut.end()).get();
                    }
                    return *ret_p;
                }
            }    // namespace detail

            template<typename T, std::size_t Arity, typename LeafIterator>
            future<nil::crypto3::containers::merkle_tree<T, Arity>> make_merkle_tree(LeafIterator first, LeafIterator last) {
                return make_ready_future<nil::crypto3::containers::merkle_tree<T, Arity>>(detail::make_merkle_tree<
                    typename std::conditional<nil::crypto3::detail::is_hash<T>::value,
                                              nil::crypto3::containers::detail::merkle_tree_node<T>,
                                              T>::type,
                    Arity>(first, last));
            }

        }    // namespace containers
    }        // namespace actor
}    // namespace nil

#endif    // ACTOR_MERKLE_TREE_HPP
