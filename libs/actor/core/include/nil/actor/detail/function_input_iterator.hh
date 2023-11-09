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

        template<typename Function, typename State>
        struct function_input_iterator {
            Function _func;
            State _state;

        public:
            function_input_iterator(Function func, State state) : _func(func), _state(state) {
            }
            function_input_iterator(const function_input_iterator &) = default;
            function_input_iterator(function_input_iterator &&) = default;
            function_input_iterator &operator=(const function_input_iterator &) = default;
            function_input_iterator &operator=(function_input_iterator &&) = default;
            auto operator*() const {
                return _func();
            }
            function_input_iterator &operator++() {
                ++_state;
                return *this;
            }
            function_input_iterator operator++(int) {
                function_input_iterator ret {*this};
                ++_state;
                return ret;
            }
            bool operator==(const function_input_iterator &x) const {
                return _state == x._state;
            }
            bool operator!=(const function_input_iterator &x) const {
                return !operator==(x);
            }
        };

        template<typename Function, typename State>
        inline function_input_iterator<Function, State> make_function_input_iterator(Function func, State state) {
            return function_input_iterator<Function, State>(func, state);
        }

        template<typename Function, typename State>
        inline function_input_iterator<Function, State> make_function_input_iterator(Function &&func) {
            return function_input_iterator<Function, State>(func, State {});
        }

    }    // namespace actor
}    // namespace nil
