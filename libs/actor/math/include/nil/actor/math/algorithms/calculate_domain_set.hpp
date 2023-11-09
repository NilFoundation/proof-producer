//---------------------------------------------------------------------------//
// Copyright (c) 2020-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
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

#ifndef ACTOR_MATH_CALCULATE_DOMAIN_SET_HPP
#define ACTOR_MATH_CALCULATE_DOMAIN_SET_HPP

#include <nil/actor/math/domains/evaluation_domain.hpp>
#include <nil/actor/math/algorithms/make_evaluation_domain.hpp>

namespace nil {
    namespace actor {
        namespace math {

            template<typename FieldType>
            future<std::vector<std::shared_ptr<evaluation_domain<FieldType>>>>
                calculate_domain_set(const std::size_t max_domain_degree, const std::size_t set_size) {
                std::vector<std::shared_ptr<evaluation_domain<FieldType>>> domain_set(set_size);
                
                std::vector<future<>> fut;
                size_t cpu_usage = std::min(set_size, (std::size_t)smp::count);
                size_t element_per_cpu = set_size / cpu_usage;

                for (auto i = 0; i < cpu_usage; ++i) {
                    auto begin = element_per_cpu * i;
                    auto end = (i == cpu_usage - 1) ? set_size : element_per_cpu * (i + 1);
                    fut.emplace_back(smp::submit_to(i, 
                        [begin, end, max_domain_degree, &domain_set]() {
                        for (std::size_t index = begin; index < end; index++) {
                            const std::size_t domain_size = std::pow(2, max_domain_degree - index);
                            domain_set[index] = make_evaluation_domain<FieldType>(domain_size);
                        }
                        return nil::actor::make_ready_future<>();
                    }));
                }
                when_all(fut.begin(), fut.end()).get();
                return nil::actor::make_ready_future<std::vector<std::shared_ptr<evaluation_domain<FieldType>>>>(domain_set);
            }
        }    // namespace math
    }        // namespace actor
}    // namespace nil

#endif    // ACTOR_MATH_CALCULATE_DOMAIN_SET_HPP
