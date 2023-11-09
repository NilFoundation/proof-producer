//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
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

#pragma once

#include <nil/actor/core/future.hh>
#include <nil/actor/detail/noncopyable_function.hh>

#include <chrono>
#include <iosfwd>

// Instrumentation to detect context switches during reactor execution
// and associated stall time, intended for use in tests

namespace nil {
    namespace actor {

        namespace detail {

            struct stall_report {
                uint64_t kernel_stalls;
                std::chrono::steady_clock::duration run_wall_time;    // excludes sleeps
                std::chrono::steady_clock::duration stall_time;
            };

            /// Run the unit-under-test (uut) function until completion, and report on any
            /// reactor stalls it generated.
            future<stall_report> report_reactor_stalls(noncopyable_function<future<>()> uut);

            std::ostream &operator<<(std::ostream &os, const stall_report &sr);

        }    // namespace detail

    }    // namespace actor
}    // namespace nil
