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

#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {

        /// \cond internal
        namespace detail {

            template<typename... Args>
            struct variant_visitor : Args... {
                variant_visitor(Args &&...a) : Args(std::move(a))... {
                }
                using Args::operator()...;
            };

            template<typename... Args>
            variant_visitor(Args &&...) -> variant_visitor<Args...>;

        }    // namespace detail
        /// \endcond

        /// \addtogroup utilities
        /// @{

        /// Creates a visitor from function objects.
        ///
        /// Returns a visitor object comprised of the provided function objects. Can be
        /// used with std::variant or any other custom variant implementation.
        ///
        /// \param args function objects each accepting one or some types stored in the variant as input
        template<typename... Args>
        auto make_visitor(Args &&...args) {
            return detail::variant_visitor<Args...>(std::forward<Args>(args)...);
        }

        /// Applies a static visitor comprised of supplied lambdas to a variant.
        /// Note that the lambdas should cover all the types that the variant can possibly hold.
        ///
        /// Returns the common type of return types of all lambdas.
        ///
        /// \tparam Variant the type of a variant
        /// \tparam Args types of lambda objects
        /// \param variant the variant object
        /// \param args lambda objects each accepting one or some types stored in the variant as input
        /// \return
        template<typename Variant, typename... Args>
        inline auto visit(Variant &&variant, Args &&...args) {
            static_assert(sizeof...(Args) > 0, "At least one lambda must be provided for visitation");
            return std::visit(make_visitor(std::forward<Args>(args)...), variant);
        }

        namespace detail {
            template<typename... Args>
            struct castable_variant {
                std::variant<Args...> var;

                template<typename... SuperArgs>
                operator std::variant<SuperArgs...>() && {
                    return std::visit([](auto &&x) { return std::variant<SuperArgs...>(std::move(x)); }, var);
                }
            };
        }    // namespace detail

        template<typename... Args>
        detail::castable_variant<Args...> variant_cast(std::variant<Args...> &&var) {
            return {std::move(var)};
        }

        template<typename... Args>
        detail::castable_variant<Args...> variant_cast(const std::variant<Args...> &var) {
            return {var};
        }

        /// @}

    }    // namespace actor
}    // namespace nil
