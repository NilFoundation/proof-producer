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

#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/timer.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/sleep.hh>

#include <chrono>
#include <iostream>

using namespace nil::actor;
using namespace std::chrono_literals;

#define BUG()                                                                \
    do {                                                                     \
        std::cerr << "ERROR @ " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error("test failed");                             \
    } while (0)

#define OK()                                                              \
    do {                                                                  \
        std::cerr << "OK @ " << __FILE__ << ":" << __LINE__ << std::endl; \
    } while (0)

template<typename Clock>
struct timer_test {
    timer<Clock> t1;
    timer<Clock> t2;
    timer<Clock> t3;
    timer<Clock> t4;
    timer<Clock> t5;
    promise<> pr1;
    promise<> pr2;

    future<> run() {
        t1.set_callback([this] {
            OK();
            fmt::print(" 500ms timer expired\n");
            if (!t4.cancel()) {
                BUG();
            }
            if (!t5.cancel()) {
                BUG();
            }
            t5.arm(1100ms);
        });
        t2.set_callback([] {
            OK();
            fmt::print(" 900ms timer expired\n");
        });
        t3.set_callback([] {
            OK();
            fmt::print("1000ms timer expired\n");
        });
        t4.set_callback([] {
            OK();
            fmt::print("  BAD cancelled timer expired\n");
        });
        t5.set_callback([this] {
            OK();
            fmt::print("1600ms rearmed timer expired\n");
            pr1.set_value();
        });

        t1.arm(500ms);
        t2.arm(900ms);
        t3.arm(1000ms);
        t4.arm(700ms);
        t5.arm(800ms);

        return pr1.get_future().then([this] { return test_timer_cancelling(); }).then([this] {
            return test_timer_with_scheduling_groups();
        });
    }

    future<> test_timer_cancelling() {
        timer<Clock> &t1 = *new timer<Clock>();
        t1.set_callback([] { BUG(); });
        t1.arm(100ms);
        t1.cancel();

        t1.arm(100ms);
        t1.cancel();

        t1.set_callback([this] {
            OK();
            pr2.set_value();
        });
        t1.arm(100ms);
        return pr2.get_future().then([&t1] { delete &t1; });
    }

    future<> test_timer_with_scheduling_groups() {
        return async([] {
            auto sg1 = create_scheduling_group("sg1", 100).get0();
            auto sg2 = create_scheduling_group("sg2", 100).get0();
            thread_attributes t1attr;
            t1attr.sched_group = sg1;
            auto expirations = 0;
            async(t1attr, [&] {
                auto make_callback_checking_sg = [&](scheduling_group sg_to_check) {
                    return [sg_to_check, &expirations] {
                        ++expirations;
                        if (current_scheduling_group() != sg_to_check) {
                            BUG();
                        }
                    };
                };
                timer<Clock> t1(make_callback_checking_sg(sg1));
                t1.arm(10ms);
                timer<Clock> t2(sg2, make_callback_checking_sg(sg2));
                t2.arm(10ms);
                sleep(500ms).get();
                if (expirations != 2) {
                    BUG();
                }
                OK();
            }).get();
            destroy_scheduling_group(sg1).get();
            destroy_scheduling_group(sg2).get();
        });
    }
};

int main(int ac, char **av) {
    app_template app;
    timer_test<steady_clock_type> t1;
    timer_test<lowres_clock> t2;
    return app.run_deprecated(ac, av, [&t1, &t2] {
        fmt::print("=== Start High res clock test\n");
        return t1.run()
            .then([&t2] {
                fmt::print("=== Start Low  res clock test\n");
                return t2.run();
            })
            .then([] {
                fmt::print("Done\n");
                engine().exit(0);
            });
    });
}
