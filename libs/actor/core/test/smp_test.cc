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
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/print.hh>

using namespace nil::actor;

future<bool> test_smp_call() {
    return smp::submit_to(1, [] { return make_ready_future<int>(3); }).then([](int ret) {
        return make_ready_future<bool>(ret == 3);
    });
}

struct nasty_exception { };

future<bool> test_smp_exception() {
    fmt::print("1\n");
    return smp::submit_to(1,
                          [] {
                              fmt::print("2\n");
                              auto x = make_exception_future<int>(nasty_exception());
                              fmt::print("3\n");
                              return x;
                          })
        .then_wrapped([](future<int> result) {
            fmt::print("4\n");
            try {
                result.get();
                return make_ready_future<bool>(false);    // expected an exception
            } catch (nasty_exception &) {
                // all is well
                return make_ready_future<bool>(true);
            } catch (...) {
                // incorrect exception type
                return make_ready_future<bool>(false);
            }
        });
}

int tests, fails;

future<> report(sstring msg, future<bool> &&result) {
    return std::move(result).then([msg](bool result) {
        fmt::print("{}: {}\n", (result ? "PASS" : "FAIL"), msg);
        tests += 1;
        fails += !result;
    });
}

int main(int ac, char **av) {
    return app_template().run_deprecated(ac, av, [] {
        return report("smp call", test_smp_call())
            .then([] { return report("smp exception", test_smp_exception()); })
            .then([] {
                fmt::print("\n{:d} tests / {:d} failures\n", tests, fails);
                engine().exit(fails ? 1 : 0);
            });
    });
}
