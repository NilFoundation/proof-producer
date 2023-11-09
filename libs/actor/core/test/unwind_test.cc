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

#define BOOST_TEST_MODULE core

#include <boost/test/included/unit_test.hpp>
#include <pthread.h>
#include <nil/actor/detail/defer.hh>
#include <nil/actor/core/posix.hh>
#include <nil/actor/detail/backtrace.hh>

using namespace nil::actor;

void foo() {
    throw std::runtime_error("foo");
}

// Exploits issue #1725
BOOST_AUTO_TEST_CASE(test_signal_mask_is_preserved_on_unwinding) {
    sigset_t mask;
    sigset_t old;
    sigfillset(&mask);
    auto res = ::pthread_sigmask(SIG_SETMASK, &mask, &old);
    throw_pthread_error(res);

    // Check which signals we actually managed to block
    res = ::pthread_sigmask(SIG_SETMASK, NULL, &mask);
    throw_pthread_error(res);

    try {
        foo();
    } catch (...) {
        // ignore
    }

    // Check backtrace()
    {
        size_t count = 0;
        backtrace([&count](auto) { ++count; });
        BOOST_REQUIRE(count > 0);
    }

    {
        sigset_t mask2;
        auto res = ::pthread_sigmask(SIG_SETMASK, &old, &mask2);
        throw_pthread_error(res);

        for (int i = 1; i < NSIG; ++i) {
            BOOST_REQUIRE(sigismember(&mask2, i) == sigismember(&mask, i));
        }
    }
}
