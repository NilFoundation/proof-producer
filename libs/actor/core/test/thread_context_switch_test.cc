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
#include <nil/actor/core/semaphore.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/distributed.hh>
#include <nil/actor/core/sleep.hh>
#include <fmt/printf.h>

using namespace nil::actor;
using namespace std::chrono_literals;

class context_switch_tester {
    uint64_t _switches {0};
    semaphore _s1 {0};
    semaphore _s2 {0};
    bool _done1 {false};
    bool _done2 {false};
    thread _t1 {[this] { main1(); }};
    thread _t2 {[this] { main2(); }};

private:
    void main1() {
        while (!_done1) {
            _s1.wait().get();
            ++_switches;
            _s2.signal();
        }
        _done2 = true;
    }
    void main2() {
        while (!_done2) {
            _s2.wait().get();
            ++_switches;
            _s1.signal();
        }
    }

public:
    void begin_measurement() {
        _s1.signal();
    }
    future<uint64_t> measure() {
        _done1 = true;
        return _t1.join().then([this] { return _t2.join(); }).then([this] { return _switches; });
    }
    future<> stop() {
        return make_ready_future<>();
    }
};

int main(int ac, char **av) {
    static const auto test_time = 5s;
    return app_template().run_deprecated(ac, av, [] {
        auto dcstp = std::make_unique<distributed<context_switch_tester>>();
        auto &dcst = *dcstp;
        return dcst.start()
            .then([&dcst] { return dcst.invoke_on_all(&context_switch_tester::begin_measurement); })
            .then([] { return sleep(test_time); })
            .then([&dcst] {
                return dcst.map_reduce0(std::mem_fn(&context_switch_tester::measure), uint64_t(),
                                        std::plus<uint64_t>());
            })
            .then([](uint64_t switches) {
                switches /= smp::count;
                fmt::print("context switch time: {:5.1f} ns\n",
                           double(std::chrono::duration_cast<std::chrono::nanoseconds>(test_time).count()) / switches);
            })
            .then([&dcst] { return dcst.stop(); })
            .then([dcstp = std::move(dcstp)] { engine_exit(0); });
    });
}
