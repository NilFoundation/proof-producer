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

namespace nil {
    namespace actor {

        template<typename Iterator, typename Func>
        class transform_iterator {
            Iterator _i;
            Func _f;

        public:
            transform_iterator(Iterator i, Func f) : _i(i), _f(f) {
            }
            auto operator*() {
                return _f(*_i);
            }
            transform_iterator &operator++() {
                ++_i;
                return *this;
            }
            transform_iterator operator++(int) {
                transform_iterator ret(*this);
                _i++;
                return ret;
            }
            bool operator==(const transform_iterator &x) const {
                return _i == x._i;
            }
            bool operator!=(const transform_iterator &x) const {
                return !operator==(x);
            }
        };

        template<typename Iterator, typename Func>
        inline transform_iterator<Iterator, Func> make_transform_iterator(Iterator i, Func f) {
            return transform_iterator<Iterator, Func>(i, f);
        }

    }    // namespace actor
}    // namespace nil
