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

#include <memory>

#include <nil/actor/core/scheduling.hh>
#include <nil/actor/detail/backtrace.hh>

namespace nil {
    namespace actor {

        class task {
            scheduling_group _sg;
#ifdef ACTOR_TASK_BACKTRACE
            shared_backtrace _bt;
#endif
        protected:
            // Task destruction is performed by run_and_dispose() via a concrete type,
            // so no need for a virtual destructor here. Derived classes that implement
            // run_and_dispose() should be declared final to avoid losing concrete type
            // information via inheritance.
            ~task() = default;

        public:
            explicit task(scheduling_group sg = current_scheduling_group()) noexcept : _sg(sg) {
            }
            virtual void run_and_dispose() noexcept = 0;
            /// Returns the next task which is waiting for this task to complete execution, or nullptr.
            virtual task *waiting_task() noexcept = 0;
            scheduling_group group() const {
                return _sg;
            }
            shared_backtrace get_backtrace() const;
#ifdef ACTOR_TASK_BACKTRACE
            void make_backtrace() noexcept;
#else
            void make_backtrace() noexcept {
            }
#endif
        };

        inline shared_backtrace task::get_backtrace() const {
#ifdef ACTOR_TASK_BACKTRACE
            return _bt;
#else
            return {};
#endif
        }

        void schedule(task *t) noexcept;
        void schedule_urgent(task *t) noexcept;

    }    // namespace actor
}    // namespace nil
