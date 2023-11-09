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

#include <nil/actor/core/semaphore.hh>
#include <nil/actor/core/loop.hh>

namespace nil {
    namespace actor {

        /// \addtogroup fiber-module
        /// @{

        /// Exception thrown when a condition variable is broken by
        /// \ref condition_variable::broken().
        class broken_condition_variable : public std::exception {
        public:
            /// Reports the exception reason.
            virtual const char *what() const noexcept;
        };

        /// Exception thrown when wait() operation times out
        /// \ref condition_variable::wait(time_point timeout).
        class condition_variable_timed_out : public std::exception {
        public:
            /// Reports the exception reason.
            virtual const char *what() const noexcept;
        };

        /// \brief Conditional variable.
        ///
        /// This is a standard computer science condition variable sans locking,
        /// since in actor access to variables is atomic anyway, adapted
        /// for futures.  You can wait for variable to be notified.
        ///
        /// To support exceptional conditions, a \ref broken() method
        /// is provided, which causes all current waiters to stop waiting,
        /// with an exceptional future returned.  This allows causing all
        /// fibers that are blocked on a condition variable to continue.
        /// This issimilar to POSIX's `pthread_cancel()`, with \ref wait()
        /// acting as a cancellation point.

        class condition_variable {
            using duration = semaphore::duration;
            using clock = semaphore::clock;
            using time_point = semaphore::time_point;
            struct condition_variable_exception_factory {
                static condition_variable_timed_out timeout() noexcept;
                static broken_condition_variable broken() noexcept;
            };
            basic_semaphore<condition_variable_exception_factory> _sem;

        public:
            /// Constructs a condition_variable object.
            /// Initialzie the semaphore with a default value of 0 to enusre
            /// the first call to wait() before signal() won't be waken up immediately.
            condition_variable() noexcept : _sem(0) {
            }

            /// Waits until condition variable is signaled, may wake up without condition been met
            ///
            /// \return a future that becomes ready when \ref signal() is called
            ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
            ///         exception.
            future<> wait() noexcept {
                return _sem.wait();
            }

            /// Waits until condition variable is signaled or timeout is reached
            ///
            /// \param timeout time point at which wait will exit with a timeout
            ///
            /// \return a future that becomes ready when \ref signal() is called
            ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
            ///         exception. If timepoint is reached will return \ref condition_variable_timed_out exception.
            future<> wait(time_point timeout) noexcept {
                return _sem.wait(timeout);
            }

            /// Waits until condition variable is signaled or timeout is reached
            ///
            /// \param timeout duration after which wait will exit with a timeout
            ///
            /// \return a future that becomes ready when \ref signal() is called
            ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
            ///         exception. If timepoint is passed will return \ref condition_variable_timed_out exception.
            future<> wait(duration timeout) noexcept {
                return _sem.wait(timeout);
            }

            /// Waits until condition variable is notified and pred() == true, otherwise
            /// wait again.
            ///
            /// \param pred predicate that checks that awaited condition is true
            ///
            /// \return a future that becomes ready when \ref signal() is called
            ///         If the condition variable was \ref broken(), may contain an exception.
            template<typename Pred>
            future<> wait(Pred &&pred) noexcept {
                return do_until(std::forward<Pred>(pred), [this] { return wait(); });
            }

            /// Waits until condition variable is notified and pred() == true or timeout is reached, otherwise
            /// wait again.
            ///
            /// \param timeout time point at which wait will exit with a timeout
            /// \param pred predicate that checks that awaited condition is true
            ///
            /// \return a future that becomes ready when \ref signal() is called
            ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
            ///         exception. If timepoint is reached will return \ref condition_variable_timed_out exception.
            template<typename Pred>
            future<> wait(time_point timeout, Pred &&pred) noexcept {
                return do_until(std::forward<Pred>(pred), [this, timeout]() mutable { return wait(timeout); });
            }

            /// Waits until condition variable is notified and pred() == true or timeout is reached, otherwise
            /// wait again.
            ///
            /// \param timeout duration after which wait will exit with a timeout
            /// \param pred predicate that checks that awaited condition is true
            ///
            /// \return a future that becomes ready when \ref signal() is called
            ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
            ///         exception. If timepoint is passed will return \ref condition_variable_timed_out exception.
            template<typename Pred>
            future<> wait(duration timeout, Pred &&pred) noexcept {
                return wait(clock::now() + timeout, std::forward<Pred>(pred));
            }
            /// Notify variable and wake up a waiter if there is one
            void signal() noexcept {
                if (_sem.waiters()) {
                    _sem.signal();
                }
            }
            /// Notify variable and wake up all waiter
            void broadcast() noexcept {
                _sem.signal(_sem.waiters());
            }

            /// Signal to waiters that an error occurred.  \ref wait() will see
            /// an exceptional future<> containing the provided exception parameter.
            /// The future is made available immediately.
            void broken() noexcept {
                _sem.broken();
            }
        };

        /// @}

    }    // namespace actor
}    // namespace nil
