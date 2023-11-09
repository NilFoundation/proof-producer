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

#include <tuple>
#include <utility>
#include <cstddef>

namespace nil {
    namespace actor {

        template<typename Func, typename Args, typename IndexList>
        struct apply_helper;

        template<typename Func, typename Tuple, size_t... I>
        struct apply_helper<Func, Tuple, std::index_sequence<I...>> {
            static auto apply(Func &&func, Tuple args) {
                return func(std::get<I>(std::forward<Tuple>(args))...);
            }
        };

        template<typename Func, typename... T>
        [[deprecated("use std::apply() instead")]] inline auto apply(Func &&func, std::tuple<T...> &&args) {
            using helper = apply_helper<Func, std::tuple<T...> &&, std::index_sequence_for<T...>>;
            return helper::apply(std::forward<Func>(func), std::move(args));
        }

        template<typename Func, typename... T>
        [[deprecated("use std::apply() instead")]] inline auto apply(Func &&func, std::tuple<T...> &args) {
            using helper = apply_helper<Func, std::tuple<T...> &, std::index_sequence_for<T...>>;
            return helper::apply(std::forward<Func>(func), args);
        }

        template<typename Func, typename... T>
        [[deprecated("use std::apply() instead")]] inline auto apply(Func &&func, const std::tuple<T...> &args) {
            using helper = apply_helper<Func, const std::tuple<T...> &, std::index_sequence_for<T...>>;
            return helper::apply(std::forward<Func>(func), args);
        }
    }    // namespace actor
}    // namespace nil
