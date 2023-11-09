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

#include <nil/actor/detail/noncopyable_function.hh>
#include <nil/actor/detail/optimized_optional.hh>
#include <nil/actor/detail/std-compat.hh>

#include <boost/intrusive/list.hpp>

#include <exception>

namespace nil {
    namespace actor {

        /// \addtogroup fiber-module
        /// @{

        /// Exception thrown when an \ref abort_source object has been
        /// notified by the \ref abort_source::request_abort() method.
        class abort_requested_exception : public std::exception {
        public:
            virtual const char *what() const noexcept override {
                return "abort requested";
            }
        };

        /// Facility to communicate a cancellation request to a fiber.
        /// Callbacks can be registered with the \c abort_source, which are called
        /// atomically with a call to request_abort().
        class abort_source {
            using subscription_callback_type = noncopyable_function<void() noexcept>;

        public:
            /// Represents a handle to the callback registered by a given fiber. Ending the
            /// lifetime of the \c subscription will unregister the callback, if it hasn't
            /// been invoked yet.
            class subscription : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
                friend class abort_source;

                subscription_callback_type _target;

                explicit subscription(abort_source &as, subscription_callback_type target) :
                    _target(std::move(target)) {
                    as._subscriptions->push_back(*this);
                }

                void on_abort() {
                    _target();
                }

            public:
                subscription() = default;

                subscription(subscription &&other) noexcept(
                    std::is_nothrow_move_constructible<subscription_callback_type>::value) :
                    _target(std::move(other._target)) {
                    subscription_list_type::node_algorithms::swap_nodes(other.this_ptr(), this_ptr());
                }

                subscription &operator=(subscription &&other) noexcept(
                    std::is_nothrow_move_assignable<subscription_callback_type>::value) {
                    if (this != &other) {
                        _target = std::move(other._target);
                        if (is_linked()) {
                            subscription_list_type::node_algorithms::unlink(this_ptr());
                        }
                        subscription_list_type::node_algorithms::swap_nodes(other.this_ptr(), this_ptr());
                    }
                    return *this;
                }

                explicit operator bool() const noexcept {
                    return is_linked();
                }
            };

        private:
            using subscription_list_type = boost::intrusive::list<subscription, boost::intrusive::constant_time_size<false>>;
            boost::optional<subscription_list_type> _subscriptions = subscription_list_type();

        public:
            /// Delays the invocation of the callback \c f until \ref request_abort() is called.
            /// \returns an engaged \ref optimized_optional containing a \ref subscription that can be used to control
            ///          the lifetime of the callback \c f, if \ref abort_requested() is \c false. Otherwise,
            ///          returns a disengaged \ref optimized_optional.
            optimized_optional<subscription> subscribe(subscription_callback_type f) noexcept(
                std::is_nothrow_move_constructible<subscription_callback_type>::value) {
                if (abort_requested()) {
                    return {};
                }
                return {subscription(*this, std::move(f))};
            }

            /// Requests that the target operation be aborted. Current subscriptions
            /// are invoked inline with this call, and no new ones can be registered.
            void request_abort() {
                _subscriptions->clear_and_dispose([](subscription *s) { s->on_abort(); });
                _subscriptions = {};
            }

            /// Returns whether an abort has been requested.
            bool abort_requested() const noexcept {
                return !_subscriptions;
            }

            /// Throws a \ref abort_requested_exception if cancellation has been requested.
            void check() const {
                if (abort_requested()) {
                    throw abort_requested_exception();
                }
            }
        };

        /// @}

    }    // namespace actor
}    // namespace nil
