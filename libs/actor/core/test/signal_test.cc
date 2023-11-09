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
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/testing/test_case.hh>

using namespace nil::actor;

extern "C" {
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
}

ACTOR_TEST_CASE(test_sighup) {
    return do_with(make_lw_shared<promise<>>(), false, [](auto const &p, bool &signaled) {
        engine().handle_signal(SIGHUP, [p, &signaled] {
            signaled = true;
            p->set_value();
        });

        kill(getpid(), SIGHUP);

        return p->get_future().then([&] { BOOST_REQUIRE_EQUAL(signaled, true); });
    });
}
