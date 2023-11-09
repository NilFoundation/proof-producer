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

#include <limits>

namespace nil {
    namespace actor {

        inline constexpr unsigned count_leading_zeros(unsigned x) {
            return __builtin_clz(x);
        }

        inline constexpr unsigned count_leading_zeros(unsigned long x) {
            return __builtin_clzl(x);
        }

        inline constexpr unsigned count_leading_zeros(unsigned long long x) {
            return __builtin_clzll(x);
        }

        inline constexpr unsigned count_trailing_zeros(unsigned x) {
            return __builtin_ctz(x);
        }

        inline constexpr unsigned count_trailing_zeros(unsigned long x) {
            return __builtin_ctzl(x);
        }

        inline constexpr unsigned count_trailing_zeros(unsigned long long x) {
            return __builtin_ctzll(x);
        }

        template<typename T>
        // requires stdx::is_integral_v<T>
        inline constexpr unsigned log2ceil(T n) {
            if (n == 1) {
                return 0;
            }
            return std::numeric_limits<T>::digits - count_leading_zeros(n - 1);
        }

        template<typename T>
        // requires stdx::is_integral_v<T>
        inline constexpr unsigned log2floor(T n) {
            return std::numeric_limits<T>::digits - count_leading_zeros(n) - 1;
        }

    }    // namespace actor
}    // namespace nil
