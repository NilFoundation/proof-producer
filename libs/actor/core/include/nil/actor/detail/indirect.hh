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

#include <functional>
#include <memory>

namespace nil {
    namespace actor {

        // This header defines functors for comparing and hashing pointers by pointed-to values instead of pointer
        // addresses.
        //
        // Examples:
        //
        //  std::multiset<shared_ptr<sstring>, indirect_less<shared_ptr<sstring>>> _multiset;
        //
        //  std::unordered_map<shared_ptr<sstring>, bool,
        //      indirect_hash<shared_ptr<sstring>>, indirect_equal_to<shared_ptr<sstring>>> _unordered_map;
        //

        template<typename Pointer, typename Equal = std::equal_to<typename std::pointer_traits<Pointer>::element_type>>
        struct indirect_equal_to {
            Equal _eq;
            indirect_equal_to(Equal eq = Equal()) : _eq(std::move(eq)) {
            }
            bool operator()(const Pointer &i1, const Pointer &i2) const {
                if (bool(i1) ^ bool(i2)) {
                    return false;
                }
                return !i1 || _eq(*i1, *i2);
            }
        };

        template<typename Pointer, typename Less = std::less<typename std::pointer_traits<Pointer>::element_type>>
        struct indirect_less {
            Less _cmp;
            indirect_less(Less cmp = Less()) : _cmp(std::move(cmp)) {
            }
            bool operator()(const Pointer &i1, const Pointer &i2) const {
                if (i1 && i2) {
                    return _cmp(*i1, *i2);
                }
                return !i1 && i2;
            }
        };

        template<typename Pointer, typename Hash = std::hash<typename std::pointer_traits<Pointer>::element_type>>
        struct indirect_hash {
            Hash _h;
            indirect_hash(Hash h = Hash()) : _h(std::move(h)) {
            }
            size_t operator()(const Pointer &p) const {
                if (p) {
                    return _h(*p);
                }
                return 0;
            }
        };

    }    // namespace actor
}    // namespace nil
