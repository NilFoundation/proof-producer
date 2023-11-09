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

#include <nil/actor/core/cacheline.hh>
#include <nil/actor/core/timer.hh>

#include <cstdint>

#include <atomic>
#include <chrono>

namespace nil {
    namespace actor {

        //
        // Forward declarations.
        //

        class lowres_clock;
        class lowres_system_clock;

        /// \cond internal

        class lowres_clock_impl final {
        public:
            using base_steady_clock = std::chrono::steady_clock;
            using base_system_clock = std::chrono::system_clock;

            // The clocks' resolutions are 10 ms. However, to make it is easier to do calculations with
            // `std::chrono::milliseconds`, we make the clock period 1 ms instead of 10 ms.
            using period = std::ratio<1, 1000>;

            using steady_rep = base_steady_clock::rep;
            using steady_duration = std::chrono::duration<steady_rep, period>;
            using steady_time_point = std::chrono::time_point<lowres_clock, steady_duration>;

            using system_rep = base_system_clock::rep;
            using system_duration = std::chrono::duration<system_rep, period>;
            using system_time_point = std::chrono::time_point<lowres_system_clock, system_duration>;

            static steady_time_point steady_now() noexcept {
                auto const nr = counters::_steady_now.load(std::memory_order_relaxed);
                return steady_time_point(steady_duration(nr));
            }

            static system_time_point system_now() noexcept {
                auto const nr = counters::_system_now.load(std::memory_order_relaxed);
                return system_time_point(system_duration(nr));
            }

            // For construction.
            friend class smp;

        private:
            // Both counters are updated by cpu0 and read by other cpus. Place them on their own cache line to avoid
            // false sharing.
            struct alignas(nil::actor::cache_line_size) counters final {
                static std::atomic<steady_rep> _steady_now;
                static std::atomic<system_rep> _system_now;
            };

            // The timer expires every 10 ms.
            static constexpr std::chrono::milliseconds _granularity {10};

            // High-resolution timer to drive these low-resolution clocks.
            timer<> _timer {};

            static void update() noexcept;

            // Private to ensure that static variables are only initialized once.
            // might throw when arming timer.
            lowres_clock_impl();
        };

        /// \endcond

        //
        /// \brief Low-resolution and efficient steady clock.
        ///
        /// This is a monotonic clock with a granularity of 10 ms. Time points from this clock do not correspond to
        /// system time.
        ///
        /// The primary benefit of this clock is that invoking \c now() is inexpensive compared to
        /// \c std::chrono::steady_clock::now().
        ///
        /// \see \c lowres_system_clock for a low-resolution clock which produces time points corresponding to system
        /// time.
        ///
        class lowres_clock final {
        public:
            using rep = lowres_clock_impl::steady_rep;
            using period = lowres_clock_impl::period;
            using duration = lowres_clock_impl::steady_duration;
            using time_point = lowres_clock_impl::steady_time_point;

            static constexpr bool is_steady = true;

            ///
            /// \note Outside of a =nil; Actor application, the result is undefined.
            ///
            static time_point now() noexcept {
                return lowres_clock_impl::steady_now();
            }
        };

        ///
        /// \brief Low-resolution and efficient system clock.
        ///
        /// This clock has the same granularity as \c lowres_clock, but it is not required to be monotonic and its time
        /// points correspond to system time.
        ///
        /// The primary benefit of this clock is that invoking \c now() is inexpensive compared to
        /// \c std::chrono::system_clock::now().
        ///
        class lowres_system_clock final {
        public:
            using rep = lowres_clock_impl::system_rep;
            using period = lowres_clock_impl::period;
            using duration = lowres_clock_impl::system_duration;
            using time_point = lowres_clock_impl::system_time_point;

            static constexpr bool is_steady = lowres_clock_impl::base_system_clock::is_steady;

            ///
            /// \note Outside of a =nil; Actor application, the result is undefined.
            ///
            static time_point now() noexcept {
                return lowres_clock_impl::system_now();
            }

            static std::time_t to_time_t(time_point t) noexcept {
                return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
            }

            static time_point from_time_t(std::time_t t) noexcept {
                return time_point(std::chrono::duration_cast<duration>(std::chrono::seconds(t)));
            }
        };

        extern template class timer<lowres_clock>;

    }    // namespace actor
}    // namespace nil
