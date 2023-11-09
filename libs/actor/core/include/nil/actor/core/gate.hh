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

#include <exception>

#include <boost/optional.hpp>

#include <nil/actor/core/future.hh>

namespace nil {
    namespace actor {

        /// \addtogroup fiber-module
        /// @{

        /// Exception thrown when a \ref gate object has been closed
        /// by the \ref gate::close() method.
        class gate_closed_exception : public std::exception {
        public:
            virtual const char *what() const noexcept override {
                return "gate closed";
            }
        };

        /// Facility to stop new requests, and to tell when existing requests are done.
        ///
        /// When stopping a service that serves asynchronous requests, we are faced with
        /// two problems: preventing new requests from coming in, and knowing when existing
        /// requests have completed.  The \c gate class provides a solution.
        class gate {
            size_t _count = 0;
            boost::optional<promise<>> _stopped;

        public:
            /// Tries to register an in-progress request.
            ///
            /// If the gate is not closed, the request is registered and the function returns `true`,
            /// Otherwise the function just returns `false` and has no other effect.
            bool try_enter() noexcept {
                bool opened = !_stopped;
                if (opened) {
                    ++_count;
                }
                return opened;
            }
            /// Registers an in-progress request.
            ///
            /// If the gate is not closed, the request is registered.  Otherwise,
            /// a \ref gate_closed_exception is thrown.
            void enter() {
                if (!try_enter()) {
                    throw gate_closed_exception();
                }
            }
            /// Unregisters an in-progress request.
            ///
            /// If the gate is closed, and there are no more in-progress requests,
            /// the `_stopped` promise will be fulfilled.
            void leave() noexcept {
                --_count;
                if (!_count && _stopped) {
                    _stopped->set_value();
                }
            }
            /// Potentially stop an in-progress request.
            ///
            /// If the gate is already closed, a \ref gate_closed_exception is thrown.
            /// By using \ref enter() and \ref leave(), the program can ensure that
            /// no further requests are serviced. However, long-running requests may
            /// continue to run. The check() method allows such a long operation to
            /// voluntarily stop itself after the gate is closed, by making calls to
            /// check() in appropriate places. check() with throw an exception and
            /// bail out of the long-running code if the gate is closed.
            void check() {
                if (_stopped) {
                    throw gate_closed_exception();
                }
            }
            /// Closes the gate.
            ///
            /// Future calls to \ref enter() will fail with an exception, and when
            /// all current requests call \ref leave(), the returned future will be
            /// made ready.
            future<> close() noexcept {
                assert(!_stopped && "nil::actor::gate::close() cannot be called more than once");
                _stopped = boost::make_optional(promise<>());
                if (!_count) {
                    _stopped->set_value();
                }
                return _stopped->get_future();
            }

            /// Returns a current number of registered in-progress requests.
            size_t get_count() const noexcept {
                return _count;
            }

            /// Returns whether the gate is closed.
            bool is_closed() const noexcept {
                return bool(_stopped);
            }
        };

        namespace detail {

            template<typename Func>
            inline auto invoke_func_with_gate(gate &g, Func &&func) noexcept {
                return futurize_invoke(std::forward<Func>(func)).finally([&g] { g.leave(); });
            }

        }    // namespace detail

        /// Executes the function \c func making sure the gate \c g is properly entered
        /// and later on, properly left.
        ///
        /// \param func function to be executed
        /// \param g the gate. Caller must make sure that it outlives this function.
        /// \returns whatever \c func returns
        ///
        /// \relates gate
        template<typename Func>
        inline auto with_gate(gate &g, Func &&func) {
            g.enter();
            return detail::invoke_func_with_gate(g, std::forward<Func>(func));
        }

        /// Executes the function \c func if the gate \c g can be entered
        /// and later on, properly left.
        ///
        /// \param func function to be executed
        /// \param g the gate. Caller must make sure that it outlives this function.
        ///
        /// If the gate is already closed, an exception future holding
        /// \ref gate_closed_exception is returned, otherwise
        /// \returns whatever \c func returns.
        ///
        /// \relates gate
        template<typename Func>
        inline auto try_with_gate(gate &g, Func &&func) noexcept {
            if (!g.try_enter()) {
                using futurator = futurize<std::result_of_t<Func()>>;
                return futurator::make_exception_future(gate_closed_exception());
            }
            return detail::invoke_func_with_gate(g, std::forward<Func>(func));
        }
        /// @}
    }    // namespace actor
}    // namespace nil
