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

#include <nil/actor/core/future.hh>
#include <nil/actor/core/circular_buffer.hh>

namespace nil {
    namespace actor {

        /// \addtogroup fiber-module
        /// @{

        /// \brief Shared/exclusive mutual exclusion.
        ///
        /// Similar to \c std::shared_mutex, this class provides protection
        /// for a shared resource, with two levels of access protection: shared
        /// and exclusive.  Shared access allows multiple tasks to access the
        /// shared resource concurrently, while exclusive access allows just
        /// one task to access the resource at a time.
        ///
        /// Note that many actor tasks do not require protection at all,
        /// since the actor scheduler is not preemptive; however tasks that do
        /// (by waiting on a future) may require explicit locking.
        ///
        /// The \ref with_shared(shared_mutex&, Func&&) and
        /// \ref with_lock(shared_mutex&, Func&&) provide exception-safe
        /// wrappers for use with \c shared_mutex.
        ///
        /// \see semaphore simpler mutual exclusion
        class shared_mutex {
            unsigned _readers = 0;
            bool _writer = false;
            struct waiter {
                waiter(promise<> &&pr, bool for_write) : pr(std::move(pr)), for_write(for_write) {
                }
                promise<> pr;
                bool for_write;
            };
            circular_buffer<waiter> _waiters;

        public:
            shared_mutex() = default;
            shared_mutex(shared_mutex &&) = default;
            shared_mutex &operator=(shared_mutex &&) = default;
            shared_mutex(const shared_mutex &) = delete;
            void operator=(const shared_mutex &) = delete;
            /// Lock the \c shared_mutex for shared access
            ///
            /// \return a future that becomes ready when no exclusive access
            ///         is granted to anyone.
            future<> lock_shared() {
                if (try_lock_shared()) {
                    return make_ready_future<>();
                }
                _waiters.emplace_back(promise<>(), false);
                return _waiters.back().pr.get_future();
            }
            /// Try to lock the \c shared_mutex for shared access
            ///
            /// \return true iff could acquire the lock for shared access.
            bool try_lock_shared() noexcept {
                if (!_writer && _waiters.empty()) {
                    ++_readers;
                    return true;
                }
                return false;
            }
            /// Unlocks a \c shared_mutex after a previous call to \ref lock_shared().
            void unlock_shared() {
                assert(_readers > 0);
                --_readers;
                wake();
            }
            /// Lock the \c shared_mutex for exclusive access
            ///
            /// \return a future that becomes ready when no access, shared or exclusive
            ///         is granted to anyone.
            future<> lock() {
                if (try_lock()) {
                    return make_ready_future<>();
                }
                _waiters.emplace_back(promise<>(), true);
                return _waiters.back().pr.get_future();
            }
            /// Try to lock the \c shared_mutex for exclusive access
            ///
            /// \return true iff could acquire the lock for exclusive access.
            bool try_lock() noexcept {
                if (!_readers && !_writer) {
                    _writer = true;
                    return true;
                }
                return false;
            }
            /// Unlocks a \c shared_mutex after a previous call to \ref lock().
            void unlock() {
                assert(_writer);
                _writer = false;
                wake();
            }

        private:
            void wake() {
                while (!_waiters.empty()) {
                    auto &w = _waiters.front();
                    // note: _writer == false in wake()
                    if (w.for_write) {
                        if (!_readers) {
                            _writer = true;
                            w.pr.set_value();
                            _waiters.pop_front();
                        }
                        break;
                    } else {    // for read
                        ++_readers;
                        w.pr.set_value();
                        _waiters.pop_front();
                    }
                }
            }
        };

        /// Executes a function while holding shared access to a resource.
        ///
        /// Executes a function while holding shared access to a resource.  When
        /// the function returns, the mutex is automatically unlocked.
        ///
        /// \param sm a \ref shared_mutex guarding access to the shared resource
        /// \param func callable object to invoke while the mutex is held for shared access
        /// \return whatever \c func returns, as a future
        ///
        /// \relates shared_mutex
        template<typename Func>
        inline futurize_t<std::result_of_t<Func()>> with_shared(shared_mutex &sm, Func &&func) {
            return sm.lock_shared().then([&sm, func = std::forward<Func>(func)]() mutable {
                return futurize_invoke(func).finally([&sm] { sm.unlock_shared(); });
            });
        }

        /// Executes a function while holding exclusive access to a resource.
        ///
        /// Executes a function while holding exclusive access to a resource.  When
        /// the function returns, the mutex is automatically unlocked.
        ///
        /// \param sm a \ref shared_mutex guarding access to the shared resource
        /// \param func callable object to invoke while the mutex is held for shared access
        /// \return whatever \c func returns, as a future
        ///
        /// \relates shared_mutex
        template<typename Func>
        inline futurize_t<std::result_of_t<Func()>> with_lock(shared_mutex &sm, Func &&func) {
            return sm.lock().then([&sm, func = std::forward<Func>(func)]() mutable {
                return futurize_invoke(func).finally([&sm] { sm.unlock(); });
            });
        }

        /// @}

    }    // namespace actor
}    // namespace nil
