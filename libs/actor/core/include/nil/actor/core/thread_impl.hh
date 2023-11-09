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

#include <nil/actor/core/preempt.hh>

#include <csetjmp>
#if defined(__APPLE__)
#define _XOPEN_SOURCE
#endif
#include <ucontext.h>
#include <chrono>

#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {
        /// Clock used for scheduling threads
        using thread_clock = std::chrono::steady_clock;

        /// \cond internal
        class thread_context;
        class scheduling_group;

        struct jmp_buf_link {
#ifdef ACTOR_ASAN_ENABLED
            ucontext_t context;
            void *fake_stack = nullptr;
            const void *stack_bottom;
            size_t stack_size;
#else
            jmp_buf jmpbuf;
#endif
            jmp_buf_link *link;
            thread_context *thread;

        public:
            void initial_switch_in(ucontext_t *initial_context, const void *stack_bottom, size_t stack_size);
            void switch_in();
            void switch_out();
            void initial_switch_in_completed();
            void final_switch_out();
        };

        extern thread_local jmp_buf_link *g_current_context;

        namespace thread_impl {

            inline thread_context *get() {
                return g_current_context->thread;
            }

            inline bool should_yield() {
                return need_preempt();
            }

            scheduling_group sched_group(const thread_context *);

            void yield();
            void switch_in(thread_context *to);
            void switch_out(thread_context *from);
            void init();

        }    // namespace thread_impl
    }        // namespace actor
}    // namespace nil
/// \endcond
