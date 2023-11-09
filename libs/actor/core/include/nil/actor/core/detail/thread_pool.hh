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

#include <nil/actor/core/detail/syscall_work_queue.hh>

namespace nil {
    namespace actor {

        class reactor;

        class thread_pool {
            reactor *_reactor;
            uint64_t _aio_threaded_fallbacks = 0;
#ifndef HAVE_OSV
            syscall_work_queue inter_thread_wq;
            posix_thread _worker_thread;
            std::atomic<bool> _stopped = {false};
            std::atomic<bool> _main_thread_idle = {false};

        public:
            explicit thread_pool(reactor *r, const sstring &thread_name);
            ~thread_pool();
            template<typename T, typename Func>
            future<T> submit(Func func) noexcept {
                ++_aio_threaded_fallbacks;
                return inter_thread_wq.submit<T>(std::move(func));
            }
            uint64_t operation_count() const {
                return _aio_threaded_fallbacks;
            }

            unsigned complete() {
                return inter_thread_wq.complete();
            }
            // Before we enter interrupt mode, we must make sure that the syscall thread will properly
            // generate signals to wake us up. This means we need to make sure that all modifications to
            // the pending and completed fields in the inter_thread_wq are visible to all threads.
            //
            // Simple release-acquire won't do because we also need to serialize all writes that happens
            // before the syscall thread loads this value, so we'll need full seq_cst.
            void enter_interrupt_mode() {
                _main_thread_idle.store(true, std::memory_order_seq_cst);
            }
            // When we exit interrupt mode, however, we can safely used relaxed order. If any reordering
            // takes place, we'll get an extra signal and complete will be called one extra time, which is
            // harmless.
            void exit_interrupt_mode() {
                _main_thread_idle.store(false, std::memory_order_relaxed);
            }

#else
        public:
            template<typename T, typename Func>
            future<T> submit(Func func) {
                std::cerr << "thread_pool not yet implemented on osv\n";
                abort();
            }
#endif
        private:
            void work(const sstring &thread_name);
        };

    }    // namespace actor
}    // namespace nil
