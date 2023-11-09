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

#include <boost/intrusive/list.hpp>

namespace nil {
    namespace actor {

        /// A non-owning reference to an object.
        ///
        /// weak_ptr allows one to keep a non-owning reference to an object. When the
        /// object is destroyed, it notifies all weak_ptr instances pointing to it.
        /// A weak_ptr instance pointing to a destroyed object is equivalent to a
        /// `nullptr`.
        ///
        /// The referenced object must inherit from weakly_referencable.
        /// weak_ptr instances can only be obtained by calling weak_from_this() on
        /// the to-be-referenced object.
        ///
        /// \see weakly_referencable
        template<typename T>
        class weak_ptr {
            template<typename U>
            friend class weakly_referencable;

        private:
            using hook_type =
                boost::intrusive::list_member_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;
            hook_type _hook;
            T *_ptr = nullptr;
            weak_ptr(T *p) noexcept : _ptr(p) {
            }

        public:
            // Note: The default constructor's body is implemented as no-op
            // rather than `noexcept = default` due to a bug with gcc 9.3.1
            // that deletes the constructor since boost::intrusive::list_member_hook
            // is not default_nothrow_constructible.
            weak_ptr() noexcept {
            }
            weak_ptr(std::nullptr_t) noexcept : weak_ptr() {
            }
            weak_ptr(weak_ptr &&o) noexcept : _ptr(o._ptr) {
                _hook.swap_nodes(o._hook);
                o._ptr = nullptr;
            }
            weak_ptr &operator=(weak_ptr &&o) noexcept {
                if (this != &o) {
                    this->~weak_ptr();
                    new (this) weak_ptr(std::move(o));
                }
                return *this;
            }
            explicit operator bool() const noexcept {
                return _ptr != nullptr;
            }
            T *operator->() const noexcept {
                return _ptr;
            }
            T &operator*() const noexcept {
                return *_ptr;
            }
            T *get() const noexcept {
                return _ptr;
            }
            bool operator==(const weak_ptr &o) const noexcept {
                return _ptr == o._ptr;
            }
            bool operator!=(const weak_ptr &o) const noexcept {
                return _ptr != o._ptr;
            }
        };

        /// Allows obtaining a non-owning reference (weak_ptr) to the object.
        ///
        /// A live weak_ptr object doesn't prevent the referenced object form being destroyed.
        ///
        /// The underlying pointer held by weak_ptr is valid as long as the referenced object is alive.
        /// When the object dies, all weak_ptr objects associated with it are emptied.
        ///
        /// A weak reference is obtained like this:
        ///
        ///     class X : public weakly_referencable<X> {};
        ///     auto x = std::make_unique<X>();
        ///     weak_ptr<X> ptr = x->weak_from_this();
        ///
        /// The user of weak_ptr can check if it still holds a valid pointer like this:
        ///
        ///     if (ptr) ptr->do_something();
        ///
        template<typename T>
        class weakly_referencable {
            boost::intrusive::list<
                weak_ptr<T>,
                boost::intrusive::member_hook<weak_ptr<T>, typename weak_ptr<T>::hook_type, &weak_ptr<T>::_hook>,
                boost::intrusive::constant_time_size<false>>
                _ptr_list;

        public:
            // Note: The default constructor's body is implemented as no-op
            // rather than `noexcept = default` due to a bug with gcc 9.3.1
            // that deletes the constructor since boost::intrusive::member_hook
            // is not default_nothrow_constructible.
            weakly_referencable() noexcept {
            }
            weakly_referencable(weakly_referencable &&) =
                delete;    // pointer to this is captured and passed to weak_ptr
            weakly_referencable(const weakly_referencable &) = delete;
            ~weakly_referencable() noexcept {
                _ptr_list.clear_and_dispose([](weak_ptr<T> *wp) noexcept { wp->_ptr = nullptr; });
            }
            weak_ptr<T> weak_from_this() noexcept {
                weak_ptr<T> ptr(static_cast<T *>(this));
                _ptr_list.push_back(ptr);
                return ptr;
            }
        };

    }    // namespace actor
}    // namespace nil
