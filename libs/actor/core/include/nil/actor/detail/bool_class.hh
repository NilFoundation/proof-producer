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

#include <ostream>

namespace nil {
    namespace actor {

        /// \addtogroup utilities
        /// @{

        /// \brief Type-safe boolean
        ///
        /// bool_class objects are type-safe boolean values that cannot be implicitly
        /// casted to untyped bools, integers or different bool_class types while still
        /// provides all relevant logical and comparison operators.
        ///
        /// bool_class template parameter is a tag type that is going to be used to
        /// distinguish booleans of different types.
        ///
        /// Usage examples:
        /// \code
        /// struct foo_tag { };
        /// using foo = bool_class<foo_tag>;
        ///
        /// struct bar_tag { };
        /// using bar = bool_class<bar_tag>;
        ///
        /// foo v1 = foo::yes; // OK
        /// bar v2 = foo::yes; // ERROR, no implicit cast
        /// foo v4 = v1 || foo::no; // OK
        /// bar v5 = bar::yes && bar(true); // OK
        /// bool v6 = v5; // ERROR, no implicit cast
        /// \endcode
        ///
        /// \tparam Tag type used as a tag
        template<typename Tag>
        class bool_class {
            bool _value;

        public:
            static const bool_class yes;
            static const bool_class no;

            /// Constructs a bool_class object initialised to \c false.
            constexpr bool_class() noexcept : _value(false) {
            }

            /// Constructs a bool_class object initialised to \c v.
            constexpr explicit bool_class(bool v) noexcept : _value(v) {
            }

            /// Casts a bool_class object to an untyped \c bool.
            explicit operator bool() const noexcept {
                return _value;
            }

            /// Logical OR.
            friend bool_class operator||(bool_class x, bool_class y) noexcept {
                return bool_class(x._value || y._value);
            }

            /// Logical AND.
            friend bool_class operator&&(bool_class x, bool_class y) noexcept {
                return bool_class(x._value && y._value);
            }

            /// Logical NOT.
            friend bool_class operator!(bool_class x) noexcept {
                return bool_class(!x._value);
            }

            /// Equal-to operator.
            friend bool operator==(bool_class x, bool_class y) noexcept {
                return x._value == y._value;
            }

            /// Not-equal-to operator.
            friend bool operator!=(bool_class x, bool_class y) noexcept {
                return x._value != y._value;
            }

            /// Prints bool_class value to an output stream.
            friend std::ostream &operator<<(std::ostream &os, bool_class v) {
                return os << (v._value ? "true" : "false");
            }
        };

        template<typename Tag>
        const bool_class<Tag> bool_class<Tag>::yes {true};
        template<typename Tag>
        const bool_class<Tag> bool_class<Tag>::no {false};

        /// @}

    }    // namespace actor
}    // namespace nil
