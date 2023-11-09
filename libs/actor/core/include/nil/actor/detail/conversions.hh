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

#include <cstdlib>
#include <string>
#include <vector>

namespace nil {
    namespace actor {

        // Convert a string to a memory size, allowing binary SI
        // suffixes (intentionally, even though SI suffixes are
        // decimal, to follow existing usage).
        //
        // "5" -> 5
        // "4k" -> (4 << 10)
        // "8M" -> (8 << 20)
        // "7G" -> (7 << 30)
        // "1T" -> (1 << 40)
        // anything else: exception
        size_t parse_memory_size(std::string s);

        static inline std::vector<char> string2vector(std::string str) {
            auto v = std::vector<char>(str.begin(), str.end());
            v.push_back('\0');
            return v;
        }

    }    // namespace actor
}    // namespace nil
