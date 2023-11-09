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

#include <cstdint>
#include <cstdlib>

namespace nil {
    namespace actor {

        template<typename T>
        inline constexpr T align_up(T v, T align) {
            return (v + align - 1) & ~(align - 1);
        }

        template<typename T>
        inline constexpr T *align_up(T *v, size_t align) {
            static_assert(sizeof(T) == 1, "align byte pointers only");
            return reinterpret_cast<T *>(align_up(reinterpret_cast<uintptr_t>(v), align));
        }

        template<typename T>
        inline constexpr T align_down(T v, T align) {
            return v & ~(align - 1);
        }

        template<typename T>
        inline constexpr T *align_down(T *v, size_t align) {
            static_assert(sizeof(T) == 1, "align byte pointers only");
            return reinterpret_cast<T *>(align_down(reinterpret_cast<uintptr_t>(v), align));
        }
    }    // namespace actor
}    // namespace nil
