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

#include <array>
#include <string>

namespace nil {
    namespace actor {

        // unordered_map implemented as a simple array

        template<typename Value, size_t Max>
        class array_map {
            std::array<Value, Max> _a {};

        public:
            array_map(std::initializer_list<std::pair<size_t, Value>> i) {
                for (auto kv : i) {
                    _a[kv.first] = kv.second;
                }
            }
            Value &operator[](size_t key) {
                return _a[key];
            }
            const Value &operator[](size_t key) const {
                return _a[key];
            }

            Value &at(size_t key) {
                if (key >= Max) {
                    throw std::out_of_range(std::to_string(key) + " >= " + std::to_string(Max));
                }
                return _a[key];
            }
        };

    }    // namespace actor
}    // namespace nil
