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

#include <type_traits>
#include <utility>

namespace nil {
    namespace actor {

        template<typename Func>
        class deferred_action {
            Func _func;
            bool _cancelled = false;

        public:
            static_assert(std::is_nothrow_move_constructible<Func>::value, "Func(Func&&) must be noexcept");
            deferred_action(Func &&func) noexcept : _func(std::move(func)) {
            }
            deferred_action(deferred_action &&o) noexcept : _func(std::move(o._func)), _cancelled(o._cancelled) {
                o._cancelled = true;
            }
            deferred_action &operator=(deferred_action &&o) noexcept {
                if (this != &o) {
                    this->~deferred_action();
                    new (this) deferred_action(std::move(o));
                }
                return *this;
            }
            deferred_action(const deferred_action &) = delete;
            ~deferred_action() {
                if (!_cancelled) {
                    _func();
                };
            }
            void cancel() {
                _cancelled = true;
            }
        };

        template<typename Func>
        inline deferred_action<Func> defer(Func &&func) {
            return deferred_action<Func>(std::forward<Func>(func));
        }

    }    // namespace actor
}    // namespace nil
