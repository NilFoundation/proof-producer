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

namespace nil {
    namespace actor {

        namespace detail {

            struct preemption_monitor {
                // We preempt when head != tail
                // This happens to match the Linux aio completion ring, so we can have the
                // kernel preempt a task by queuing a completion event to an io_context.
                std::atomic<uint32_t> head;
                std::atomic<uint32_t> tail;
            };

        }    // namespace detail

        extern __thread const detail::preemption_monitor *g_need_preempt;

        inline bool need_preempt() noexcept {
#ifndef ACTOR_DEBUG
            // prevent compiler from eliminating loads in a loop
            std::atomic_signal_fence(std::memory_order_seq_cst);
            auto np = g_need_preempt;
            // We aren't reading anything from the ring, so we don't need
            // any barriers.
            auto head = np->head.load(std::memory_order_relaxed);
            auto tail = np->tail.load(std::memory_order_relaxed);
            // Possible optimization: read head and tail in a single 64-bit load,
            // and find a funky way to compare the two 32-bit halves.
            return __builtin_expect(head != tail, false);
#else
            return true;
#endif
        }
    }    // namespace actor
}    // namespace nil
