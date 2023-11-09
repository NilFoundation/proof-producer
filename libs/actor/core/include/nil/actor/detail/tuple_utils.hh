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

        /// \cond internal
        namespace detail {

            template<typename Tuple>
            Tuple untuple(Tuple t) {
                return t;
            }

            template<typename T>
            T untuple(std::tuple<T> t) {
                return std::get<0>(std::move(t));
            }

            template<typename Tuple, typename Function, size_t... I>
            void tuple_for_each_helper(Tuple &&t, Function &&f, std::index_sequence<I...> &&) {
                auto ignore_me = {(f(std::get<I>(std::forward<Tuple>(t))), 1)...};
                (void)ignore_me;
            }

            template<typename Tuple, typename MapFunction, size_t... I>
            auto tuple_map_helper(Tuple &&t, MapFunction &&f, std::index_sequence<I...> &&) {
                return std::make_tuple(f(std::get<I>(std::forward<Tuple>(t)))...);
            }

            template<size_t I, typename IndexSequence>
            struct prepend;

            template<size_t I, size_t... Is>
            struct prepend<I, std::index_sequence<Is...>> {
                using type = std::index_sequence<I, Is...>;
            };

            template<template<typename> class Filter, typename Tuple, typename IndexSequence>
            struct tuple_filter;

            template<template<typename> class Filter, typename T, typename... Ts, size_t I, size_t... Is>
            struct tuple_filter<Filter, std::tuple<T, Ts...>, std::index_sequence<I, Is...>> {
                using tail = typename tuple_filter<Filter, std::tuple<Ts...>, std::index_sequence<Is...>>::type;
                using type = std::conditional_t<Filter<T>::value, typename prepend<I, tail>::type, tail>;
            };

            template<template<typename> class Filter>
            struct tuple_filter<Filter, std::tuple<>, std::index_sequence<>> {
                using type = std::index_sequence<>;
            };

            template<typename Tuple, size_t... I>
            auto tuple_filter_helper(Tuple &&t, std::index_sequence<I...> &&) {
                return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
            }

        }    // namespace detail
        /// \endcond

        /// \addtogroup utilities
        /// @{

        /// Applies type transformation to all types in tuple
        ///
        /// Member type `type` is set to a tuple type which is a result of applying
        /// transformation `MapClass<T>::type` to each element `T` of the input tuple
        /// type.
        ///
        /// \tparam MapClass class template defining type transformation
        /// \tparam Tuple input tuple type
        template<template<typename> class MapClass, typename Tuple>
        struct tuple_map_types;

        /// @}

        template<template<typename> class MapClass, typename... Elements>
        struct tuple_map_types<MapClass, std::tuple<Elements...>> {
            using type = std::tuple<typename MapClass<Elements>::type...>;
        };

        /// \addtogroup utilities
        /// @{

        /// Filters elements in tuple by their type
        ///
        /// Returns a tuple containing only those elements which type `T` caused
        /// expression FilterClass<T>::value to be true.
        ///
        /// \tparam FilterClass class template having an element value set to true for elements that
        ///                     should be present in the result
        /// \param t tuple to filter
        /// \return a tuple contaning elements which type passed the test
        template<template<typename> class FilterClass, typename... Elements>
        auto tuple_filter_by_type(const std::tuple<Elements...> &t) {
            using sequence = typename detail::tuple_filter<FilterClass, std::tuple<Elements...>,
                                                           std::index_sequence_for<Elements...>>::type;
            return detail::tuple_filter_helper(t, sequence());
        }
        template<template<typename> class FilterClass, typename... Elements>
        auto tuple_filter_by_type(std::tuple<Elements...> &&t) {
            using sequence = typename detail::tuple_filter<FilterClass, std::tuple<Elements...>,
                                                           std::index_sequence_for<Elements...>>::type;
            return detail::tuple_filter_helper(std::move(t), sequence());
        }

        /// Applies function to all elements in tuple
        ///
        /// Applies given function to all elements in the tuple and returns a tuple
        /// of results.
        ///
        /// \param t original tuple
        /// \param f function to apply
        /// \return tuple of results returned by f for each element in t
        template<typename Function, typename... Elements>
        auto tuple_map(const std::tuple<Elements...> &t, Function &&f) {
            return detail::tuple_map_helper(t, std::forward<Function>(f), std::index_sequence_for<Elements...>());
        }
        template<typename Function, typename... Elements>
        auto tuple_map(std::tuple<Elements...> &&t, Function &&f) {
            return detail::tuple_map_helper(std::move(t), std::forward<Function>(f),
                                            std::index_sequence_for<Elements...>());
        }

        /// Iterate over all elements in tuple
        ///
        /// Iterates over given tuple and calls the specified function for each of
        /// it elements.
        ///
        /// \param t a tuple to iterate over
        /// \param f function to call for each tuple element
        template<typename Function, typename... Elements>
        void tuple_for_each(const std::tuple<Elements...> &t, Function &&f) {
            return detail::tuple_for_each_helper(t, std::forward<Function>(f), std::index_sequence_for<Elements...>());
        }
        template<typename Function, typename... Elements>
        void tuple_for_each(std::tuple<Elements...> &t, Function &&f) {
            return detail::tuple_for_each_helper(t, std::forward<Function>(f), std::index_sequence_for<Elements...>());
        }
        template<typename Function, typename... Elements>
        void tuple_for_each(std::tuple<Elements...> &&t, Function &&f) {
            return detail::tuple_for_each_helper(std::move(t), std::forward<Function>(f),
                                                 std::index_sequence_for<Elements...>());
        }

        /// @}

    }    // namespace actor
}    // namespace nil
