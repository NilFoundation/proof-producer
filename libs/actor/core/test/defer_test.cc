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
#include <nil/actor/detail/defer.hh>

using namespace nil::actor;

BOOST_AUTO_TEST_CASE(test_defer_does_not_run_when_canceled) {
    bool ran = false;
    {
        auto d = defer([&] { ran = true; });
        d.cancel();
    }
    BOOST_REQUIRE(!ran);
}

BOOST_AUTO_TEST_CASE(test_defer_runs) {
    bool ran = false;
    {
        auto d = defer([&] { ran = true; });
    }
    BOOST_REQUIRE(ran);
}

BOOST_AUTO_TEST_CASE(test_defer_runs_once_when_moved) {
    int ran = 0;
    {
        auto d = defer([&] { ++ran; });
        { auto d2 = std::move(d); }
        BOOST_REQUIRE_EQUAL(1, ran);
    }
    BOOST_REQUIRE_EQUAL(1, ran);
}

BOOST_AUTO_TEST_CASE(test_defer_does_not_run_when_moved_after_cancelled) {
    int ran = 0;
    {
        auto d = defer([&] { ++ran; });
        d.cancel();
        { auto d2 = std::move(d); }
    }
    BOOST_REQUIRE_EQUAL(0, ran);
}
