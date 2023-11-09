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

#include <nil/actor/testing/perf_tests.hh>

#include <nil/actor/core/sharded.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/fair_queue.hh>
#include <nil/actor/core/semaphore.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/core/when_all.hh>

#include <boost/range/irange.hpp>

struct local_fq_and_class {
    nil::actor::fair_group fg;
    nil::actor::fair_queue fq;
    nil::actor::fair_queue sfq;
    nil::actor::priority_class_ptr pclass;
    unsigned executed = 0;

    nil::actor::fair_queue &queue(bool local) noexcept {
        return local ? fq : sfq;
    }

    local_fq_and_class(nil::actor::fair_group &sfg) :
        fg(nil::actor::fair_group::config(1, 1)), fq(fg, nil::actor::fair_queue::config()),
        sfq(sfg, nil::actor::fair_queue::config()), pclass(fq.register_priority_class(1)) {
    }

    ~local_fq_and_class() {
        fq.unregister_priority_class(pclass);
    }
};

struct local_fq_entry {
    nil::actor::fair_queue_entry ent;
    std::function<void()> submit;

    template<typename Func>
    local_fq_entry(unsigned weight, unsigned index, Func &&f) :
        ent(nil::actor::fair_queue_ticket(weight, index)), submit(std::move(f)) {
    }
};

struct perf_fair_queue {

    static constexpr unsigned requests_to_dispatch = 1000;

    nil::actor::sharded<local_fq_and_class> local_fq;

    nil::actor::fair_group shared_fg;

    perf_fair_queue() : shared_fg(nil::actor::fair_group::config(smp::count, smp::count)) {
        local_fq.start(std::ref(shared_fg)).get();
    }

    ~perf_fair_queue() {
        local_fq.stop().get();
    }

    future<> test(bool local);
};

future<> perf_fair_queue::test(bool loc) {

    auto invokers = local_fq.invoke_on_all([loc](local_fq_and_class &local) {
        return parallel_for_each(boost::irange(0u, requests_to_dispatch), [&local, loc](unsigned dummy) {
            auto req = std::make_unique<local_fq_entry>(1, 1, [&local, loc] {
                local.executed++;
                local.queue(loc).notify_requests_finished(nil::actor::fair_queue_ticket {1, 1});
            });
            local.queue(loc).queue(local.pclass, req->ent);
            req.release();
            return make_ready_future<>();
        });
    });

    auto collectors = local_fq.invoke_on_all([loc](local_fq_and_class &local) {
        // Zeroing this counter must be here, otherwise should the collectors win the
        // execution order in when_all_succeed(), the do_until()'s stopping callback
        // would return true immediately and the queue would not be dispatched.
        //
        // At the same time, although this counter is incremented by the lambda from
        // invokers, it's not called until the fq.dispatch_requests() is, so there's no
        // opposite problem if zeroing it here.
        local.executed = 0;

        return do_until([&local] { return local.executed == requests_to_dispatch; },
                        [&local, loc] {
                            local.queue(loc).dispatch_requests([](fair_queue_entry &ent) {
                                local_fq_entry *le =
                                    boost::intrusive::get_parent_from_member(&ent, &local_fq_entry::ent);
                                le->submit();
                                delete le;
                            });
                            return make_ready_future<>();
                        });
    });

    return when_all_succeed(std::move(invokers), std::move(collectors)).discard_result();
}

PERF_TEST_F(perf_fair_queue, contended_local) {
    return test(true);
}
PERF_TEST_F(perf_fair_queue, contended_shared) {
    return test(false);
}
