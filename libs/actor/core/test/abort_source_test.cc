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

#include <nil/actor/core/gate.hh>
#include <nil/actor/core/sleep.hh>
#include <nil/actor/core/do_with.hh>

using namespace nil::actor;
using namespace std::chrono_literals;

ACTOR_TEST_CASE(test_abort_source_notifies_subscriber) {
    bool signalled = false;
    auto as = abort_source();
    auto st_opt = as.subscribe([&signalled]() noexcept { signalled = true; });
    BOOST_REQUIRE_EQUAL(true, bool(st_opt));
    as.request_abort();
    BOOST_REQUIRE_EQUAL(true, signalled);
    BOOST_REQUIRE_EQUAL(false, bool(st_opt));
    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_abort_source_subscription_unregister) {
    bool signalled = false;
    auto as = abort_source();
    auto st_opt = as.subscribe([&signalled]() noexcept { signalled = true; });
    BOOST_REQUIRE_EQUAL(true, bool(st_opt));
    st_opt = {};
    as.request_abort();
    BOOST_REQUIRE_EQUAL(false, signalled);
    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_abort_source_rejects_subscription) {
    auto as = abort_source();
    as.request_abort();
    auto st_opt = as.subscribe([]() noexcept {});
    BOOST_REQUIRE_EQUAL(false, bool(st_opt));
    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_sleep_abortable) {
    auto as = std::make_unique<abort_source>();
    auto f = sleep_abortable(100s, *as).then_wrapped([](auto &&f) {
        try {
            f.get();
            BOOST_FAIL("should have failed");
        } catch (const sleep_aborted &e) {
            // expected
        } catch (...) {
            BOOST_FAIL("unexpected exception");
        }
    });
    as->request_abort();
    return f.finally([as = std::move(as)] {});
}

// Verify that negative sleep does not sleep forever. It should not sleep
// at all.
ACTOR_TEST_CASE(test_negative_sleep_abortable) {
    return do_with(abort_source(), [](abort_source &as) { return sleep_abortable(-10s, as); });
}
