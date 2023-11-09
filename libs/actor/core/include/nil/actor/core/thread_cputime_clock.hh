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

#include <chrono>
#include <time.h>
#include <cassert>

namespace nil {
    namespace actor {

        class thread_cputime_clock {
        public:
            using rep = int64_t;
            using period = std::chrono::nanoseconds::period;
            using duration = std::chrono::duration<rep, period>;
            using time_point = std::chrono::time_point<thread_cputime_clock, duration>;

        public:
            static time_point now() {
                using namespace std::chrono_literals;

                struct timespec tp;
                [[gnu::unused]] auto ret = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tp);
                assert(ret == 0);
                return time_point(tp.tv_nsec * 1ns + tp.tv_sec * 1s);
            }
        };
    }    // namespace actor
}    // namespace nil
