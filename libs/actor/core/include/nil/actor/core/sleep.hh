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

#include <chrono>
#include <functional>

#include <nil/actor/core/abort_source.hh>
#include <nil/actor/core/future.hh>
#include <nil/actor/core/lowres_clock.hh>
#include <nil/actor/core/timer.hh>

namespace nil {
    namespace actor {

        /// \file

        /// Returns a future which completes after a specified time has elapsed.
        ///
        /// \param dur minimum amount of time before the returned future becomes
        ///            ready.
        /// \return A \ref future which becomes ready when the sleep duration elapses.
        template<typename Clock = steady_clock_type, typename Rep, typename Period>
        future<> sleep(std::chrono::duration<Rep, Period> dur) {
            struct sleeper {
                promise<> done;
                timer<Clock> tmr;
                sleeper(std::chrono::duration<Rep, Period> dur) : tmr([this] { done.set_value(); }) {
                    tmr.arm(dur);
                }
            };
            sleeper *s = new sleeper(dur);
            future<> fut = s->done.get_future();
            return fut.then([s] { delete s; });
        }

        /// exception that is thrown when application is in process of been stopped
        class sleep_aborted : public std::exception {
        public:
            /// Reports the exception reason.
            virtual const char *what() const noexcept {
                return "Sleep is aborted";
            }
        };

        /// Returns a future which completes after a specified time has elapsed
        /// or throws \ref sleep_aborted exception if application is aborted
        ///
        /// \param dur minimum amount of time before the returned future becomes
        ///            ready.
        /// \return A \ref future which becomes ready when the sleep duration elapses.
        template<typename Clock = steady_clock_type>
        future<> sleep_abortable(typename Clock::duration dur);

        extern template future<> sleep_abortable<steady_clock_type>(typename steady_clock_type::duration);
        extern template future<> sleep_abortable<lowres_clock>(typename lowres_clock::duration);

        /// Returns a future which completes after a specified time has elapsed
        /// or throws \ref sleep_aborted exception if the sleep is aborted.
        ///
        /// \param dur minimum amount of time before the returned future becomes
        ///            ready.
        /// \param as the \ref abort_source that eventually notifies that the sleep
        ///            should be aborted.
        /// \return A \ref future which becomes ready when the sleep duration elapses.
        template<typename Clock = steady_clock_type>
        future<> sleep_abortable(typename Clock::duration dur, abort_source &as);

        extern template future<> sleep_abortable<steady_clock_type>(typename steady_clock_type::duration,
                                                                    abort_source &);
        extern template future<> sleep_abortable<lowres_clock>(typename lowres_clock::duration, abort_source &);

    }    // namespace actor
}    // namespace nil
