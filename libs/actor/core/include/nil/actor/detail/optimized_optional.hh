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

#include <boost/config.hpp>
#include <boost/optional.hpp>

#include <nil/actor/detail/std-compat.hh>

#include <type_traits>
#include <iostream>

namespace nil {
    namespace actor {

#ifdef BOOST_HAS_CONCEPTS

        template<typename T>
        concept OptimizableOptional = std::is_default_constructible<T>::value &&
            std::is_nothrow_move_assignable<T>::value && requires(const T &obj) {
            { bool(obj) }
            noexcept;
        };

#endif

        /// \c optimized_optional<> is intended mainly for use with classes that store
        /// their data externally and expect pointer to this data to be always non-null.
        /// In such case there is no real need for another flag signifying whether
        /// the optional is engaged.
        template<typename T>
        class optimized_optional {
            T _object;

        public:
            optimized_optional() = default;
            optimized_optional(boost::none_t) noexcept {
            }
            optimized_optional(const T &obj) : _object(obj) {
            }
            optimized_optional(T &&obj) noexcept : _object(std::move(obj)) {
            }
            optimized_optional(boost::optional<T> &&obj) noexcept {
                if (obj) {
                    _object = std::move(*obj);
                }
            }
            optimized_optional(const optimized_optional &) = default;
            optimized_optional(optimized_optional &&) = default;

            optimized_optional &operator=(boost::none_t) noexcept {
                _object = T();
                return *this;
            }
            template<typename U>
            std::enable_if_t<std::is_same<std::decay_t<U>, T>::value, optimized_optional &>
                operator=(U &&obj) noexcept {
                _object = std::forward<U>(obj);
                return *this;
            }
            optimized_optional &operator=(const optimized_optional &) = default;
            optimized_optional &operator=(optimized_optional &&) = default;

            explicit operator bool() const noexcept {
                return bool(_object);
            }

            T *operator->() noexcept {
                return &_object;
            }
            const T *operator->() const noexcept {
                return &_object;
            }

            T &operator*() noexcept {
                return _object;
            }
            const T &operator*() const noexcept {
                return _object;
            }

            bool operator==(const optimized_optional &other) const {
                return _object == other._object;
            }
            bool operator!=(const optimized_optional &other) const {
                return _object != other._object;
            }
            friend std::ostream &operator<<(std::ostream &out, const optimized_optional &opt) {
                if (!opt) {
                    return out << "null";
                }
                return out << *opt;
            }
        };

    }    // namespace actor
}    // namespace nil
