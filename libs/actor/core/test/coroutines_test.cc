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

#include <nil/actor/core/future-util.hh>
#include <nil/actor/testing/test_case.hh>

using namespace nil::actor;

#ifndef ACTOR_COROUTINES_ENABLED

ACTOR_TEST_CASE(test_coroutines_not_compiled_in) {
    return make_ready_future<>();
}

#else

#include <nil/actor/core/coroutine.hh>

namespace {

    future<int> old_fashioned_continuations() {
        return later().then([] { return 42; });
    }

    future<int> simple_coroutine() {
        co_await later();
        co_return 53;
    }

    future<int> ready_coroutine() {
        co_return 64;
    }

    future<std::tuple<int, double>> tuple_coroutine() {
        co_return std::tuple(1, 2.);
    }

    future<int> failing_coroutine() {
        co_await later();
        throw 42;
    }

}    // namespace

ACTOR_TEST_CASE(test_simple_coroutines) {
    BOOST_REQUIRE_EQUAL(co_await old_fashioned_continuations(), 42);
    BOOST_REQUIRE_EQUAL(co_await simple_coroutine(), 53);
    BOOST_REQUIRE_EQUAL(ready_coroutine().get0(), 64);
    BOOST_REQUIRE(co_await tuple_coroutine() == std::tuple(1, 2.));
    BOOST_REQUIRE_EXCEPTION((void)co_await failing_coroutine(), int, [](auto v) { return v == 42; });
}

ACTOR_TEST_CASE(test_abandond_coroutine) {
    boost::optional<future<int>> f;
    {
        auto p1 = promise<>();
        auto p2 = promise<>();
        auto p3 = promise<>();
        f = p1.get_future().then([&]() -> future<int> {
            p2.set_value();
            BOOST_CHECK_THROW(co_await p3.get_future(), broken_promise);
            co_return 1;
        });
        p1.set_value();
        co_await p2.get_future();
    }
    BOOST_CHECK_EQUAL(co_await std::move(*f), 1);
}

ACTOR_TEST_CASE(test_scheduling_group) {
    auto other_sg = co_await create_scheduling_group("the other group", 10.f);

    auto p1 = promise<>();
    auto p2 = promise<>();

    auto p1b = promise<>();
    auto p2b = promise<>();
    auto f1 = p1b.get_future();
    auto f2 = p2b.get_future();

    BOOST_REQUIRE(current_scheduling_group() == default_scheduling_group());
    auto f_ret = with_scheduling_group(
        other_sg,
        [other_sg_cap = other_sg](future<> f1, future<> f2, promise<> p1, promise<> p2) -> future<int> {
            // Make a copy in the coroutine before the lambda is destroyed.
            auto other_sg = other_sg_cap;
            BOOST_REQUIRE(current_scheduling_group() == other_sg);
            BOOST_REQUIRE(other_sg == other_sg);
            p1.set_value();
            co_await std::move(f1);
            BOOST_REQUIRE(current_scheduling_group() == other_sg);
            p2.set_value();
            co_await std::move(f2);
            BOOST_REQUIRE(current_scheduling_group() == other_sg);
            co_return 42;
        },
        p1.get_future(), p2.get_future(), std::move(p1b), std::move(p2b));

    co_await std::move(f1);
    BOOST_REQUIRE(current_scheduling_group() == default_scheduling_group());
    p1.set_value();
    co_await std::move(f2);
    BOOST_REQUIRE(current_scheduling_group() == default_scheduling_group());
    p2.set_value();
    BOOST_REQUIRE_EQUAL(co_await std::move(f_ret), 42);
    BOOST_REQUIRE(current_scheduling_group() == default_scheduling_group());
}

ACTOR_TEST_CASE(test_preemption) {
    bool x = false;
    unsigned preempted = 0;
    auto f = later().then([&x] { x = true; });

    // try to preempt 1000 times. 1 should be enough if not for
    // task queue shaffling in debug mode which may cause co-routine
    // continuation to run first.
    while (preempted < 1000 && !x) {
        preempted += need_preempt();
        co_await make_ready_future<>();
    }
    auto save_x = x;
    // wait for later() to complete
    co_await std::move(f);
    BOOST_REQUIRE(save_x);
    co_return;
}
#endif
