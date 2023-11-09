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

namespace nil {
    namespace actor {

        template<typename T>
        struct function_traits;

        template<typename Ret, typename... Args>
        struct function_traits<Ret(Args...)> {
            using return_type = Ret;
            using args_as_tuple = std::tuple<Args...>;
            using signature = Ret(Args...);

            static constexpr std::size_t arity = sizeof...(Args);

            template<std::size_t N>
            struct arg {
                static_assert(N < arity, "no such parameter index.");
                using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
            };
        };

        template<typename Ret, typename... Args>
        struct function_traits<Ret (*)(Args...)> : public function_traits<Ret(Args...)> { };

        template<typename T, typename Ret, typename... Args>
        struct function_traits<Ret (T::*)(Args...)> : public function_traits<Ret(Args...)> { };

        template<typename T, typename Ret, typename... Args>
        struct function_traits<Ret (T::*)(Args...) const> : public function_traits<Ret(Args...)> { };

        template<typename T>
        struct function_traits : public function_traits<decltype(&T::operator())> { };

        template<typename T>
        struct function_traits<T &> : public function_traits<std::remove_reference_t<T>> { };

    }    // namespace actor
}    // namespace nil
