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

/// \file
/// \brief Contains a nil::actor::checked_ptr class implementation.

#include <exception>

#include <boost/config.hpp>

/// \namespace nil::actor
namespace nil {
    namespace actor {

        /// The exception thrown by a default_null_deref_action.
        class checked_ptr_is_null_exception : public std::exception { };

        /// \brief
        /// Default not engaged nil::actor::checked_ptr dereferencing action (functor).
        ///
        /// Throws a nil::actor::checked_ptr_is_null_exception.
        ///
        struct default_null_deref_action {
            /// \throw nil::actor::checked_ptr_is_null_exception
            void operator()() const {
                throw checked_ptr_is_null_exception();
            }
        };

        /// \cond internal
        /// \namespace nil::actor::internal
        namespace detail {

            /// \name nil::actor::checked_ptr::get() helpers
            /// Helper functions that simplify the nil::actor::checked_ptr::get() implementation.
            /// @{

#ifdef BOOST_HAS_CONCEPTS
            /// Invokes the get() method of a smart pointer object.
            /// \param ptr A smart pointer object
            /// \return A pointer to the underlying object
            template<typename T>
            /// cond ACTOR_CONCEPT_DOC - nested '\ cond' doesn't seem to work (bug 736553), so working it around
            requires requires(T ptr) {
                ptr.get();
            }
            /// endcond
            inline typename std::pointer_traits<std::remove_const_t<T>>::element_type *checked_ptr_do_get(T &ptr) {
                return ptr.get();
            }
#else
            /// Invokes the get() method of a smart pointer object.
            /// \param ptr A smart pointer object
            /// \return A pointer to the underlying object
            template<typename T>
            inline typename std::pointer_traits<std::remove_const_t<T>>::element_type *checked_ptr_do_get(T &ptr) {
                return ptr.get();
            }
#endif

            /// Return a pointer itself for a naked pointer argument.
            /// \param ptr A naked pointer object
            /// \return An input naked pointer object
            template<typename T>
            inline T *checked_ptr_do_get(T *ptr) noexcept {
                return ptr;
            }
            /// @}
        }    // namespace detail
        /// \endcond

        /// \class nil::actor::checked_ptr
        /// \brief
        /// nil::actor::checked_ptr class is a wrapper class that may be used with any pointer type
        /// (smart like std::unique_ptr or raw pointers like int*).
        ///
        /// The nil::actor::checked_ptr object will invoke the NullDerefAction functor if
        /// it is dereferenced when the underlying pointer is not engaged.
        ///
        /// It may still be assigned, compared to other nil::actor::checked_ptr objects or
        /// moved without limitations.
        ///
        /// The default NullDerefAction will throw a  nil::actor::default_null_deref_action exception.
        ///
        /// \tparam NullDerefAction a functor that is invoked when a user tries to dereference a not engaged pointer.
        ///
        template<typename Ptr, typename NullDerefAction = default_null_deref_action>
#ifdef BOOST_HAS_CONCEPTS
        /// \cond ACTOR_CONCEPT_DOC
        requires std::is_default_constructible<NullDerefAction>::value && requires(NullDerefAction action) {
            NullDerefAction();
        }
        /// \endcond
#endif
        class checked_ptr {
        public:
            /// Underlying element type
            using element_type = typename std::pointer_traits<Ptr>::element_type;

            /// Type of the pointer to the underlying element
            using pointer = element_type *;

        private:
            Ptr _ptr = nullptr;

        private:
            /// Invokes a NullDerefAction functor if the underlying pointer is not engaged.
            void check() const {
                if (!_ptr) {
                    NullDerefAction()();
                }
            }

        public:
            checked_ptr() noexcept(noexcept(Ptr(nullptr))) = default;
            checked_ptr(std::nullptr_t) noexcept(
                std::is_nothrow_default_constructible<checked_ptr<Ptr, NullDerefAction>>::value) :
                checked_ptr() {
            }
            checked_ptr(Ptr &&ptr) noexcept(std::is_nothrow_move_constructible<Ptr>::value) : _ptr(std::move(ptr)) {
            }
            checked_ptr(const Ptr &p) noexcept(std::is_nothrow_copy_constructible<Ptr>::value) : _ptr(p) {
            }

            /// \name Checked Methods
            /// These methods start with invoking a NullDerefAction functor if the underlying pointer is not engaged.
            /// @{

            /// Invokes the get() method of the underlying smart pointer or returns the pointer itself for a raw pointer
            /// (const variant). \return The pointer to the underlying object
            pointer get() const {
                check();
                return detail::checked_ptr_do_get(_ptr);
            }

            /// Gets a reference to the underlying pointer object.
            /// \return The underlying pointer object
            const Ptr &operator->() const {
                check();
                return _ptr;
            }

            /// Gets a reference to the underlying pointer object (const variant).
            /// \return The underlying pointer object
            Ptr &operator->() {
                check();
                return _ptr;
            }

            /// Gets the reference to the underlying object (const variant).
            /// \return The reference to the underlying object
            const element_type &operator*() const {
                check();
                return *_ptr;
            }

            /// Gets the reference to the underlying object.
            /// \return The reference to the underlying object
            element_type &operator*() {
                check();
                return *_ptr;
            }
            /// @}

            /// \name Unchecked methods
            /// These methods may be invoked when the underlying pointer is not engaged.
            /// @{

            /// Checks if the underlying pointer is engaged.
            /// \return TRUE if the underlying pointer is engaged
            explicit operator bool() const {
                return bool(_ptr);
            }

            bool operator==(const checked_ptr &other) const {
                return _ptr == other._ptr;
            }
            bool operator!=(const checked_ptr &other) const {
                return _ptr != other._ptr;
            }

            /// Gets the hash value for the underlying pointer object.
            /// \return The hash value for the underlying pointer object
            size_t hash() const {
                return std::hash<Ptr>()(_ptr);
            }
            ///@}
        };

    }    // namespace actor
}    // namespace nil

namespace std {
    /// std::hash specialization for nil::actor::checked_ptr class
    template<typename T>
    struct hash<nil::actor::checked_ptr<T>> {
        /// Get the hash value for the given nil::actor::checked_ptr object.
        /// The hash will calculated using the nil::actor::checked_ptr::hash method.
        /// \param p object for hash value calculation
        /// \return The hash value for the given object
        size_t operator()(const nil::actor::checked_ptr<T> &p) const {
            return p.hash();
        }
    };
}    // namespace std
