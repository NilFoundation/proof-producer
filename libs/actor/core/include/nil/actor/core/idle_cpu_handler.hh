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

/// \file

namespace nil {
    namespace actor {

        /// Indicates the outcome of a user callback installed to take advantage of
        /// idle CPU cycles.
        enum class idle_cpu_handler_result {
            no_more_work,                          //!< The user callback has no more work to perform
            interrupted_by_higher_priority_task    //!< A call to the work_waiting_on_reactor parameter to
                                                   //!< idle_cpu_handler returned `true`
        };

        /// Signature of a callback provided by the reactor to a user callback installed to take
        /// advantage of idle cpu cycles, used to periodically check if the CPU is still idle.
        ///
        /// \return true if the reactor has new work to do
        using work_waiting_on_reactor = const noncopyable_function<bool()> &;

        /// Signature of a callback provided by the user, that the reactor calls when it has idle cycles.
        ///
        /// The `poll` parameter is a work_waiting_on_reactor function that should be periodically called
        /// to check if the idle callback should return with
        /// idle_cpu_handler_result::interrupted_by_higher_priority_task
        using idle_cpu_handler = noncopyable_function<idle_cpu_handler_result(work_waiting_on_reactor poll)>;

        /// Set a handler that will be called when there is no task to execute on cpu.
        /// Handler should do a low priority work.
        ///
        /// Handler's return value determines whether handler did any actual work. If no work was done then reactor will
        /// go into sleep.
        ///
        /// Handler's argument is a function that returns true if a task which should be executed on cpu appears or
        /// false otherwise. This function should be used by a handler to return early if a task appears.
        void set_idle_cpu_handler(idle_cpu_handler &&handler);

    }    // namespace actor
}    // namespace nil
