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

#include <nil/actor/core/thread.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <nil/actor/testing/test_runner.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/when_all.hh>
#include <nil/actor/core/file.hh>
#include <nil/actor/core/io_queue.hh>
#include <nil/actor/core/io_intent.hh>
#include <nil/actor/core/detail/io_request.hh>
#include <nil/actor/core/detail/io_sink.hh>

using namespace nil::actor;

template<size_t Len>
struct fake_file {
    int data[Len] = {};

    static detail::io_request make_write_req(size_t idx, int val) {
        int *buf = new int(val);
        return detail::io_request::make_write(0, idx, buf, 1);
    }

    void execute_write_req(detail::io_request &rq, io_completion *desc) {
        data[rq.pos()] = *(reinterpret_cast<int *>(rq.address()));
        desc->complete_with(rq.size());
    }
};

struct io_queue_for_tests {
    io_group_ptr group;
    detail::io_sink sink;
    io_queue queue;

    io_queue_for_tests() :
        group(std::make_shared<io_group>(io_group::config {})), sink(), queue(group, sink, io_queue::config {0}) {
    }
};

ACTOR_THREAD_TEST_CASE(test_basic_flow) {
    io_queue_for_tests tio;
    fake_file<1> file;

    auto f = tio.queue.queue_request(default_priority_class(), 0, file.make_write_req(0, 42), nullptr)
                 .then([&file](size_t len) { BOOST_REQUIRE(file.data[0] == 42); });

    tio.queue.poll_io_queue();
    tio.sink.drain([&file](detail::io_request &rq, io_completion *desc) -> bool {
        file.execute_write_req(rq, desc);
        return true;
    });

    f.get();
}

ACTOR_THREAD_TEST_CASE(test_intent_safe_ref) {
    auto get_cancelled = [](detail::intent_reference &iref) -> bool {
        try {
            iref.retrieve();
            return false;
        } catch (nil::actor::cancelled_error &err) {
            return true;
        }
    };

    io_intent intent, intent_x;

    detail::intent_reference ref_orig(&intent);
    BOOST_REQUIRE(ref_orig.retrieve() == &intent);

    // Test move armed
    detail::intent_reference ref_armed(std::move(ref_orig));
    BOOST_REQUIRE(ref_orig.retrieve() == nullptr);
    BOOST_REQUIRE(ref_armed.retrieve() == &intent);

    detail::intent_reference ref_armed_2(&intent_x);
    ref_armed_2 = std::move(ref_armed);
    BOOST_REQUIRE(ref_armed.retrieve() == nullptr);
    BOOST_REQUIRE(ref_armed_2.retrieve() == &intent);

    intent.cancel();
    BOOST_REQUIRE(get_cancelled(ref_armed_2));

    // Test move cancelled
    detail::intent_reference ref_cancelled(std::move(ref_armed_2));
    BOOST_REQUIRE(ref_armed_2.retrieve() == nullptr);
    BOOST_REQUIRE(get_cancelled(ref_cancelled));

    detail::intent_reference ref_cancelled_2(&intent_x);
    ref_cancelled_2 = std::move(ref_cancelled);
    BOOST_REQUIRE(ref_cancelled.retrieve() == nullptr);
    BOOST_REQUIRE(get_cancelled(ref_cancelled_2));

    // Test move empty
    detail::intent_reference ref_empty(std::move(ref_orig));
    BOOST_REQUIRE(ref_empty.retrieve() == nullptr);

    detail::intent_reference ref_empty_2(&intent_x);
    ref_empty_2 = std::move(ref_empty);
    BOOST_REQUIRE(ref_empty_2.retrieve() == nullptr);
}

static constexpr int nr_requests = 24;

ACTOR_THREAD_TEST_CASE(test_io_cancellation) {
    fake_file<nr_requests> file;

    io_queue_for_tests tio;
    io_priority_class pc0 = tio.queue.register_one_priority_class("a", 100);
    io_priority_class pc1 = tio.queue.register_one_priority_class("b", 100);

    size_t idx = 0;
    int val = 100;

    io_intent live, dead;

    std::vector<future<>> finished;
    std::vector<future<>> cancelled;

    auto queue_legacy_request = [&](io_queue_for_tests &q, io_priority_class &pc) {
        auto f =
            q.queue.queue_request(pc, 0, file.make_write_req(idx, val), nullptr).then([&file, idx, val](size_t len) {
                BOOST_REQUIRE(file.data[idx] == val);
                return make_ready_future<>();
            });
        finished.push_back(std::move(f));
        idx++;
        val++;
    };

    auto queue_live_request = [&](io_queue_for_tests &q, io_priority_class &pc) {
        auto f = q.queue.queue_request(pc, 0, file.make_write_req(idx, val), &live).then([&file, idx, val](size_t len) {
            BOOST_REQUIRE(file.data[idx] == val);
            return make_ready_future<>();
        });
        finished.push_back(std::move(f));
        idx++;
        val++;
    };

    auto queue_dead_request = [&](io_queue_for_tests &q, io_priority_class &pc) {
        auto f = q.queue.queue_request(pc, 0, file.make_write_req(idx, val), &dead)
                     .then_wrapped([](auto &&f) {
                         try {
                             f.get();
                             BOOST_REQUIRE(false);
                         } catch (...) {
                         }
                         return make_ready_future<>();
                     })
                     .then([&file, idx]() { BOOST_REQUIRE(file.data[idx] == 0); });
        cancelled.push_back(std::move(f));
        idx++;
        val++;
    };

    auto seed = std::random_device {}();
    std::default_random_engine reng(seed);
    std::uniform_int_distribution<> dice(0, 5);

    for (int i = 0; i < nr_requests; i++) {
        int pc = dice(reng) % 2;
        if (dice(reng) < 3) {
            fmt::print("queue live req to pc {}\n", pc);
            queue_live_request(tio, pc == 0 ? pc0 : pc1);
        } else if (dice(reng) < 5) {
            fmt::print("queue dead req to pc {}\n", pc);
            queue_dead_request(tio, pc == 0 ? pc0 : pc1);
        } else {
            fmt::print("queue legacy req to pc {}\n", pc);
            queue_legacy_request(tio, pc == 0 ? pc0 : pc1);
        }
    }

    dead.cancel();

    // cancelled requests must resolve right at once

    when_all_succeed(cancelled.begin(), cancelled.end()).get();

    tio.queue.poll_io_queue();
    tio.sink.drain([&file](detail::io_request &rq, io_completion *desc) -> bool {
        file.execute_write_req(rq, desc);
        return true;
    });

    when_all_succeed(finished.begin(), finished.end()).get();
}
