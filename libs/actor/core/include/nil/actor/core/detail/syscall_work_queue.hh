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

#include <nil/actor/core/detail/pollable_fd.hh>
#include <nil/actor/core/future.hh>
#include <nil/actor/core/semaphore.hh>

#include <nil/actor/detail/std-compat.hh>
#include <nil/actor/detail/noncopyable_function.hh>

#include <boost/lockfree/spsc_queue.hpp>

namespace nil {
    namespace actor {

        class syscall_work_queue {
            static constexpr size_t queue_length = 128;
            struct work_item;
            using lf_queue = boost::lockfree::spsc_queue<work_item *, boost::lockfree::capacity<queue_length>>;
            lf_queue _pending;
            lf_queue _completed;
            writeable_eventfd _start_eventfd;
            semaphore _queue_has_room = {queue_length};
            struct work_item {
                virtual ~work_item() {
                }
                virtual void process() = 0;
                virtual void complete() = 0;
                virtual void set_exception(std::exception_ptr) = 0;
            };
            template<typename T>
            struct work_item_returning : work_item {
                noncopyable_function<T()> _func;
                promise<T> _promise;
                boost::optional<T> _result;
                work_item_returning(noncopyable_function<T()> func) : _func(std::move(func)) {
                }
                virtual void process() override {
                    _result = this->_func();
                }
                virtual void complete() override {
                    _promise.set_value(std::move(*_result));
                }
                virtual void set_exception(std::exception_ptr eptr) override {
                    _promise.set_exception(eptr);
                };
                future<T> get_future() {
                    return _promise.get_future();
                }
            };

        public:
            syscall_work_queue();
            template<typename T>
            future<T> submit(noncopyable_function<T()> func) noexcept {
                try {
                    auto wi = std::make_unique<work_item_returning<T>>(std::move(func));
                    auto fut = wi->get_future();
                    submit_item(std::move(wi));
                    return fut;
                } catch (...) {
                    return current_exception_as_future<T>();
                }
            }

        private:
            void work();
            // Scans the _completed queue, that contains the requests already handled by the syscall thread,
            // effectively opening up space for more requests to be submitted. One consequence of this is
            // that from the reactor's point of view, a request is not considered handled until it is
            // removed from the _completed queue.
            //
            // Returns the number of requests handled.
            unsigned complete();
            void submit_item(std::unique_ptr<syscall_work_queue::work_item> wi);

            friend class thread_pool;
        };

    }    // namespace actor
}    // namespace nil
