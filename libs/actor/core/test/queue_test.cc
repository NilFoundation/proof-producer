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

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/core/queue.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/sleep.hh>

using namespace nil::actor;
using namespace std::chrono_literals;

ACTOR_TEST_CASE(test_queue_pop_after_abort) {
    return async([] {
        queue<int> q(1);
        bool exception = false;
        bool timer = false;
        future<> done = make_ready_future();
        q.abort(std::make_exception_ptr(std::runtime_error("boom")));
        done = sleep(1ms).then([&] {
            timer = true;
            q.abort(std::make_exception_ptr(std::runtime_error("boom")));
        });
        try {
            q.pop_eventually().get();
        } catch (...) {
            exception = !timer;
        }
        BOOST_REQUIRE(exception);
        done.get();
    });
}

ACTOR_TEST_CASE(test_queue_push_abort) {
    return async([] {
        queue<int> q(1);
        bool exception = false;
        bool timer = false;
        future<> done = make_ready_future();
        q.abort(std::make_exception_ptr(std::runtime_error("boom")));
        done = sleep(1ms).then([&] {
            timer = true;
            q.abort(std::make_exception_ptr(std::runtime_error("boom")));
        });
        try {
            q.push_eventually(1).get();
        } catch (...) {
            exception = !timer;
        }
        BOOST_REQUIRE(exception);
        done.get();
    });
}
