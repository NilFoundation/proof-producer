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

#include <atomic>
#include <cassert>

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace nil {
    namespace actor {

        namespace detail {
#if defined(__x86_64__) || defined(__i386__)

            /// \brief Puts the current CPU thread into a "relaxed" state.
            ///
            /// This function is supposed to significantly improve the performance in situations like spinlocks when
            /// process spins in a tight loop waiting for a lock. The actual implementation is different on different
            /// platforms. For more details look for "Pause Intrinsic" for x86 version, and for "yield" assembly
            /// instruction documentation for Power platform.
            [[gnu::always_inline]] inline void cpu_relax() {
                _mm_pause();
            }

#elif defined(__PPC__)

            [[gnu::always_inline]] inline void cpu_relax() {
                __asm__ volatile("yield");
            }

#elif defined(__s390x__) || defined(__zarch__)

            // FIXME: there must be a better way
            [[gnu::always_inline]] inline void cpu_relax() {
            }

#elif defined(__aarch64__)

            [[gnu::always_inline]] inline void cpu_relax() {
                __asm__ volatile("yield");
            }

#else

            [[gnu::always_inline]] inline void cpu_relax() {
            }
#warn "Using an empty cpu_relax() for this architecture"

#endif

        }    // namespace detail

        namespace util {

            // Spin lock implementation.
            // BasicLockable.
            // Async-signal safe.
            // unlock() "synchronizes with" lock().
            class spinlock {
                std::atomic<bool> _busy = {false};

            public:
                spinlock() = default;
                spinlock(const spinlock &) = delete;
                ~spinlock() {
                    assert(!_busy.load(std::memory_order_relaxed));
                }
                bool try_lock() noexcept {
                    return !_busy.exchange(true, std::memory_order_acquire);
                }
                void lock() noexcept {
                    while (_busy.exchange(true, std::memory_order_acquire)) {
                        detail::cpu_relax();
                    }
                }
                void unlock() noexcept {
                    _busy.store(false, std::memory_order_release);
                }
            };

        }    // namespace util

    }    // namespace actor
}    // namespace nil
