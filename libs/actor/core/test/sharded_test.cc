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

#include <nil/actor/testing/thread_test_case.hh>

#include <nil/actor/core/sharded.hh>

using namespace nil::actor;

namespace {
    class invoke_on_during_stop final : public peering_sharded_service<invoke_on_during_stop> {
        bool flag = false;

    public:
        future<> stop() {
            return container().invoke_on(0, [](invoke_on_during_stop &instance) { instance.flag = true; });
        }

        ~invoke_on_during_stop() {
            if (this_shard_id() == 0) {
                assert(flag);
            }
        }
    };
}    // namespace

ACTOR_THREAD_TEST_CASE(invoke_on_during_stop_test) {
    sharded<invoke_on_during_stop> s;
    s.start().get();
    s.stop().get();
}

class mydata {
public:
    int x = 1;
    future<> stop() {
        return make_ready_future<>();
    }
};

ACTOR_THREAD_TEST_CASE(invoke_map_returns_non_future_value) {
    nil::actor::sharded<mydata> s;
    s.start().get();
    s.map([](mydata &m) { return m.x; })
        .then([](std::vector<int> results) {
            for (auto &x : results) {
                assert(x == 1);
            }
        })
        .get();
    s.stop().get();
};

ACTOR_THREAD_TEST_CASE(invoke_map_returns_future_value) {
    nil::actor::sharded<mydata> s;
    s.start().get();
    s.map([](mydata &m) { return make_ready_future<int>(m.x); })
        .then([](std::vector<int> results) {
            for (auto &x : results) {
                assert(x == 1);
            }
        })
        .get();
    s.stop().get();
}

ACTOR_THREAD_TEST_CASE(invoke_map_returns_future_value_from_thread) {
    nil::actor::sharded<mydata> s;
    s.start().get();
    s.map([](mydata &m) { return nil::actor::async([&m] { return m.x; }); })
        .then([](std::vector<int> results) {
            for (auto &x : results) {
                assert(x == 1);
            }
        })
        .get();
    s.stop().get();
}

ACTOR_THREAD_TEST_CASE(failed_sharded_start_doesnt_hang) {
    class fail_to_start {
    public:
        fail_to_start() {
            throw 0;
        }
    };

    nil::actor::sharded<fail_to_start> s;
    s.start().then_wrapped([](auto &&fut) { fut.ignore_ready_future(); }).get();
}
