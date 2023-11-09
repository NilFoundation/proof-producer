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

#include <nil/actor/detail/alloc_failure_injector.hh>
#include <nil/actor/detail/backtrace.hh>
#include <nil/actor/detail/log.hh>
#include <nil/actor/detail/defer.hh>

namespace nil {
    namespace actor {
        namespace memory {

            static logger log("failure_injector");

            thread_local alloc_failure_injector the_alloc_failure_injector;

            void alloc_failure_injector::fail() {
                _failed = true;
                cancel();
                if (log.is_enabled(log_level::trace)) {
                    log.trace("Failing at {}", current_backtrace());
                }
                _on_alloc_failure();
            }

            void alloc_failure_injector::run_with_callback(noncopyable_function<void()> callback,
                                                           noncopyable_function<void()>
                                                               to_run) {
                auto restore = defer([this, prev = std::exchange(_on_alloc_failure, std::move(callback))]() mutable {
                    _on_alloc_failure = std::move(prev);
                });
                to_run();
            }

            void with_allocation_failures(noncopyable_function<void()> func) {
                auto &injector = memory::local_failure_injector();
                uint64_t i = 0;
                do {
                    try {
                        injector.fail_after(i++);
                        func();
                        injector.cancel();
                    } catch (const std::bad_alloc &) {
                        // expected
                    }
                } while (injector.failed());
            }

        }    // namespace memory
    }        // namespace actor
}    // namespace nil
