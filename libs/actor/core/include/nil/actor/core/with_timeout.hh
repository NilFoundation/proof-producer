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

#include <nil/actor/core/future.hh>
#include <nil/actor/core/timed_out_error.hh>
#include <nil/actor/core/timer.hh>

namespace nil {
    namespace actor {

        /// \addtogroup future-util
        /// @{

        /// \brief Wait for either a future, or a timeout, whichever comes first
        ///
        /// When timeout is reached the returned future resolves with an exception
        /// produced by ExceptionFactory::timeout(). By default it is \ref timed_out_error exception.
        ///
        /// Note that timing out doesn't cancel any tasks associated with the original future.
        /// It also doesn't cancel the callback registerred on it.
        ///
        /// \param f future to wait for
        /// \param timeout time point after which the returned future should be failed
        ///
        /// \return a future which will be either resolved with f or a timeout exception
        template<typename ExceptionFactory = default_timeout_exception_factory, typename Clock, typename Duration,
                 typename... T>
        future<T...> with_timeout(std::chrono::time_point<Clock, Duration> timeout, future<T...> f) {
            if (f.available()) {
                return f;
            }
            auto pr = std::make_unique<promise<T...>>();
            auto result = pr->get_future();
            timer<Clock> timer([&pr = *pr] { pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout())); });
            timer.arm(timeout);
            // Future is returned indirectly.
            (void)f.then_wrapped([pr = std::move(pr), timer = std::move(timer)](auto &&f) mutable {
                if (timer.cancel()) {
                    f.forward_to(std::move(*pr));
                } else {
                    f.ignore_ready_future();
                }
            });
            return result;
        }

        /// @}

    }    // namespace actor
}    // namespace nil
