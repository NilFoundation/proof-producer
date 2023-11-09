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

        /// \addtogroup utilities
        /// @{

        /// Wrapper for lvalue references
        ///
        /// reference_wrapper wraps a lvalue reference into a copyable and assignable
        /// object. It is very similar to std::reference_wrapper except that it doesn't
        /// allow implicit construction from a reference and the only way to construct
        /// it is to use either ref() or cref(). The reason for that discrepancy (and
        /// also the reason why nil::actor::reference_wrapper was introduced) is that it
        /// server different purpose than std::reference_wrapper. The latter protects
        /// references from decaying and allows copying and assigning them.
        /// nil::actor::reference_wrapper is used mainly to force user to explicitly
        /// state that object is passed by reference thus reducing the chances that
        /// the referred object being prematurely destroyed in case the execution
        /// is deferred to a continuation.
        template<typename T>
        class reference_wrapper {
            T *_pointer;

            explicit reference_wrapper(T &object) noexcept : _pointer(&object) {
            }

            template<typename U>
            friend reference_wrapper<U> ref(U &) noexcept;
            template<typename U>
            friend reference_wrapper<const U> cref(const U &) noexcept;

        public:
            using type = T;

            operator T &() const noexcept {
                return *_pointer;
            }
            T &get() const noexcept {
                return *_pointer;
            }
        };

        /// Wraps reference in a reference_wrapper
        template<typename T>
        inline reference_wrapper<T> ref(T &object) noexcept {
            return reference_wrapper<T>(object);
        }

        /// Wraps constant reference in a reference_wrapper
        template<typename T>
        inline reference_wrapper<const T> cref(const T &object) noexcept {
            return reference_wrapper<const T>(object);
        }

        /// @}

    }    // namespace actor
}    // namespace nil
