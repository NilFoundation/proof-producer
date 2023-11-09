//---------------------------------------------------------------------------//
// Copyright (c) 2022 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
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

#ifndef ACTOR_MATH_DETAIL_HPP
#define ACTOR_MATH_DETAIL_HPP

#include <nil/actor/core/smp.hh>
#include <nil/actor/core/when_all.hh>
#include <nil/actor/core/future.hh>

namespace nil {
    namespace actor {
        namespace math {
            namespace detail {
                template<typename Func>
                future<> block_execution(std::size_t elements_count, std::size_t smp_count, Func &&func) {
                    std::vector<future<>> fut;

                    // We experimentally noticed, that when at least 4 cores are available, it's better to keep core #0 idle.
                    bool use_core_0 = (smp_count < 4);

                    std::size_t cpu_usage = std::min(elements_count, smp_count);

                    if (!use_core_0 && elements_count >= smp_count) {
                        --cpu_usage;
                    }
                    std::size_t begin = 0;

                    for (auto i = 0; i < cpu_usage; ++i) {
                        auto end = begin + (elements_count - begin) / (cpu_usage - i);
                        fut.emplace_back(smp::submit_to(i + (use_core_0 ? 0 : 1), [begin, end, func]() {
                            func(begin, end);
                            return make_ready_future<>();
                        }));
                        begin = end;
                    }

                    when_all(fut.begin(), fut.end()).get();

                    return make_ready_future<>();
                }
            }    // namespace detail
        }        // namespace math
    }            // namespace actor
}    // namespace nil

#endif    // ACTOR_MATH_DETAIL_HPP
