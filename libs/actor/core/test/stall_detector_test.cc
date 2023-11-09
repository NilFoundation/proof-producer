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

#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/thread_cputime_clock.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/detail/later.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <nil/actor/core/detail/stall_detector.hh>

#include <atomic>
#include <chrono>

using namespace nil::actor;
using namespace std::chrono_literals;

class temporary_stall_detector_settings {
    std::chrono::milliseconds _old_threshold;
    std::function<void()> _old_report;

public:
    temporary_stall_detector_settings(std::chrono::duration<double> threshold, std::function<void()> report) :
        _old_threshold(engine().get_blocked_reactor_notify_ms()),
        _old_report(engine().get_stall_detector_report_function()) {
        engine().update_blocked_reactor_notify_ms(std::chrono::duration_cast<std::chrono::milliseconds>(threshold));
        engine().set_stall_detector_report_function(std::move(report));
    }
    ~temporary_stall_detector_settings() {
        engine().update_blocked_reactor_notify_ms(_old_threshold);
        engine().set_stall_detector_report_function(std::move(_old_report));
    }
};

void spin(std::chrono::duration<double> how_much) {
    auto end = detail::cpu_stall_detector::clock_type::now() + how_much;
    while (detail::cpu_stall_detector::clock_type::now() < end) {
        // spin!
    }
}

void spin_some_cooperatively(std::chrono::duration<double> how_much) {
    auto end = std::chrono::steady_clock::now() + how_much;
    while (std::chrono::steady_clock::now() < end) {
        spin(200us);
        if (need_preempt()) {
            thread::yield();
        }
    }
}

ACTOR_THREAD_TEST_CASE(normal_case) {
    std::atomic<unsigned> reports {};
    temporary_stall_detector_settings tsds(10ms, [&] { ++reports; });
    spin_some_cooperatively(1s);
    BOOST_REQUIRE_EQUAL(reports, 0);
}

ACTOR_THREAD_TEST_CASE(simple_stalls) {
    std::atomic<unsigned> reports {};
    temporary_stall_detector_settings tsds(10ms, [&] { ++reports; });
    unsigned nr = 10;
    for (unsigned i = 0; i < nr; ++i) {
        spin_some_cooperatively(100ms);
        spin(20ms);
    }
    spin_some_cooperatively(100ms);

    // blocked-reactor-reports-per-minute defaults to 5, so we don't
    // get all 10 reports.
    BOOST_REQUIRE_EQUAL(reports, 5);
}

ACTOR_THREAD_TEST_CASE(no_poll_no_stall) {
    std::atomic<unsigned> reports {};
    temporary_stall_detector_settings tsds(10ms, [&] { ++reports; });
    spin_some_cooperatively(1ms);    // need to yield so that stall detector change from above take effect
    static constexpr unsigned tasks = 2000;
    promise<> p;
    auto f = p.get_future();
    parallel_for_each(boost::irange(0u, tasks), [&p](unsigned int i) {
        (void)later().then([i, &p] {
            spin(500us);
            if (i == tasks - 1) {
                p.set_value();
            }
        });
        return make_ready_future<>();
    }).get();
    f.get();
    BOOST_REQUIRE_EQUAL(reports, 0);
}
